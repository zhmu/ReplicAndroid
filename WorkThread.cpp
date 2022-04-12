#include "WorkThread.h"
#include <deque>
#include <PortableDevice.h>
#include <fstream>
#include <filesystem>

struct WorkItem
{
	mtp::ObjectID objectID;
	size_t size;
	std::string destPath;
};

struct PendingItem
{
	mtp::ObjectID objectID;
	std::string destPath;
	size_t size;
};

struct WorkThread::Impl
{
	WorkThread& thread;
	CComPtr<IPortableDevice> activeDevice;
	BackupLocations locations;
	std::atomic<bool> aborted;

	void Run()
	{
		std::deque<PendingItem> pendingItems;
		for (const auto& location : locations) {
			pendingItems.push_back({ location.objectId, location.where });
			std::filesystem::create_directory(location.where);
		}

		std::vector<PendingItem> itemsToTransfer;

		NumbersAvailable na{};
		while (!pendingItems.empty())
		{
			auto pendingItem = pendingItems.front();
			pendingItems.pop_front();

			auto contents = mtp::EnumerateContents(activeDevice, pendingItem.objectID);
			if (!contents) continue;

			for (const auto& objectId : *contents)
			{
				// Do not abort in the middle of a file, we want to prevent corrupting an item
				if (aborted) return;

				auto props = mtp::ReadProperties(activeDevice, objectId);
				if (!props) continue;
				if (!props->name) continue;

				const auto size = props->size ? *props->size : 0;
				auto path = pendingItem.destPath;
				path += '/';
				if (props->contentType == WPD_CONTENT_TYPE_FOLDER)
				{
					path += *props->name; // XXX remove illegal stuff
					pendingItems.push_back({ objectId, path });
					std::filesystem::create_directory(path);
					continue;
				}

				path += *props->name; // XXX remove illegal stuff
				itemsToTransfer.push_back({ objectId, path, size });

				++na.totalNumberOfItems;
				na.totalNumberOfBytes += size;
				emit thread.numbersUpdated(na);
			}
		}
		emit thread.numbersComplete(na);

		ItemsUpdate iu{};
		std::vector<FailedItem> failedItems;
		for (const auto& item : itemsToTransfer) {
			if (aborted) break;

			std::error_code ec{};
			const auto size = std::filesystem::file_size(item.destPath, ec);
			if (!ec && size == item.size) {
				++iu.itemsTransferredSkipped;
				iu.bytesSkipped += item.size;
				continue;
			}

			std::ofstream ofs(item.destPath, std::ofstream::out | std::ofstream::trunc | std::ofstream::binary);
			mtp::ExpectedOrHResult<size_t> result{S_FALSE};
			if (ofs) {
				result = mtp::ReadData(activeDevice, item.objectID, [&](const void* data, size_t length) {
					ofs.write(static_cast<const char*>(data), length);
					return static_cast<bool>(ofs);
				});
			}
			if (!result || !ofs)
			{
				failedItems.push_back({ item.destPath });
				iu.bytesSkipped += item.size;
				++iu.itemsTransferredFailures;
			}
			else
			{
				iu.bytesRead += *result;
				++iu.itemsTransferredSuccessfully;
			}
			emit thread.itemsUpdated(na, iu);
		}

		emit thread.finished(iu, failedItems);
	}
};

WorkThread::WorkThread(QObject* parent, CComPtr<IPortableDevice> activeDevice, BackupLocations locations)
	: QThread(parent)
	, impl(std::make_unique<Impl>(*this, activeDevice, std::move(locations)))
{
}

WorkThread::~WorkThread()
{
	impl->aborted = true;
	wait();
}

void WorkThread::run()
{
	impl->Run();
}

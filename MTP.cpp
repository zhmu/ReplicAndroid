#include "MTP.h"

#include <codecvt>
#include <locale>
#include "portabledeviceapi.h"
#include "portabledevice.h"

namespace mtp
{
	namespace
	{
		constexpr auto CLIENT_NAME = L"ReplicAndroid";
		constexpr auto CLIENT_MAJOR_VER = 1;
		constexpr auto CLIENT_MINOR_VER = 0;
		constexpr auto CLIENT_REVISION = 0;

		// https://stackoverflow.com/questions/4358870/convert-wstring-to-string-encoded-in-utf-8
		std::string wstring_to_utf8(const std::wstring& str)
		{
			std::wstring_convert<std::codecvt_utf8<wchar_t>> myconv;
			return myconv.to_bytes(str);
		}

		std::wstring utf8_to_wstring(const std::string& str)
		{
			std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> myconv;
			return myconv.from_bytes(str);
		}

		ExpectedOrHResult<CComPtr<IPortableDeviceValues>> GetClientInformation()
		{
			CComPtr<IPortableDeviceValues> clientInformation;

			if (const auto hr = clientInformation.CoCreateInstance(CLSID_PortableDeviceValues, NULL, CLSCTX_INPROC_SERVER); FAILED(hr)) return hr;
			if (const auto hr = clientInformation->SetStringValue(WPD_CLIENT_NAME, CLIENT_NAME); FAILED(hr)) return hr;
			if (const auto hr = clientInformation->SetUnsignedIntegerValue(WPD_CLIENT_MAJOR_VERSION, CLIENT_MAJOR_VER); FAILED(hr)) return hr;
			if (const auto hr = clientInformation->SetUnsignedIntegerValue(WPD_CLIENT_MINOR_VERSION, CLIENT_MINOR_VER); FAILED(hr)) return hr;
			if (const auto hr = clientInformation->SetUnsignedIntegerValue(WPD_CLIENT_REVISION, CLIENT_REVISION); FAILED(hr)) return hr;
			if (const auto hr = clientInformation->SetUnsignedIntegerValue(WPD_CLIENT_SECURITY_QUALITY_OF_SERVICE, SECURITY_IMPERSONATION); FAILED(hr)) return hr;

			return clientInformation;
		}
	}

	ExpectedOrHResult<std::vector<PortableDevice>> EnumeratePortableDevices()
	{
		CComPtr<IPortableDeviceManager> portableDeviceManager;
		if (const auto hr = CoCreateInstance(CLSID_PortableDeviceManager, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&portableDeviceManager)); FAILED(hr)) return hr;

		DWORD deviceCount{};
		if (const auto hr = portableDeviceManager->GetDevices(NULL, &deviceCount); FAILED(hr)) return hr;

		std::vector<PortableDevice> devices;
		if (deviceCount > 0)
		{
			auto deviceIDs = std::make_unique<PWSTR[]>(deviceCount);
			if (const auto hr = portableDeviceManager->GetDevices(deviceIDs.get(), &deviceCount); FAILED(hr)) return hr;

			for (int n = 0; n < deviceCount; ++n) {
				const auto id = deviceIDs[n];

				auto getString = [&](auto func) -> std::optional<std::string> {
					DWORD size{};
					if (const auto hr = func(nullptr, &size); SUCCEEDED(hr)) {
						if (size == 0) return "";
						auto str = std::make_unique<WCHAR[]>(size);
						if (const auto hr = func(str.get(), &size); SUCCEEDED(hr))
							return wstring_to_utf8(str.get());
					}
					return {};
				};

				auto name = getString([&](auto str, auto size) { return portableDeviceManager->GetDeviceFriendlyName(id, str, size); });
				auto manufacturer = getString([&](auto str, auto size) { return portableDeviceManager->GetDeviceManufacturer(id, str, size); });
				auto descr = getString([&](auto str, auto size) { return portableDeviceManager->GetDeviceDescription(id, str, size); });
				devices.push_back({ wstring_to_utf8(id), std::move(name), std::move(manufacturer), std::move(descr) });
				CoTaskMemFree(id);
			}
		}
		return devices;
	}

	ExpectedOrHResult<CComPtr<IPortableDevice>> OpenDevice(const DeviceID& deviceId)
	{
		CComPtr<IPortableDevice> device;
		if (const auto hr = device.CoCreateInstance(CLSID_PortableDeviceFTM, NULL, CLSCTX_INPROC_SERVER); FAILED(hr)) return hr;

		auto clientInformation = GetClientInformation();
		if (!clientInformation) return clientInformation.GetResult();

		clientInformation->SetUnsignedIntegerValue(WPD_CLIENT_DESIRED_ACCESS, GENERIC_READ);
		auto id = utf8_to_wstring(deviceId);
		if (const auto hr = device->Open(id.data(), *clientInformation); FAILED(hr)) return hr;

		return device;
	}

	ExpectedOrHResult<ObjectProperties> ReadProperties(CComPtr<IPortableDevice>& device, const ObjectID& id)
	{
		CComPtr<IPortableDeviceContent> content;
		if (const auto hr = device->Content(&content); FAILED(hr)) return hr;

		CComPtr<IPortableDeviceProperties> props;
		if (const auto hr = content->Properties(&props); FAILED(hr)) return hr;

		CComPtr<IPortableDeviceKeyCollection> propsToRead;
		if (const auto hr = propsToRead.CoCreateInstance(CLSID_PortableDeviceKeyCollection, NULL, CLSCTX_INPROC_SERVER); FAILED(hr)) return hr;

		if (const auto hr = propsToRead->Add(WPD_OBJECT_PARENT_ID); FAILED(hr)) return hr;
		if (const auto hr = propsToRead->Add(WPD_OBJECT_NAME); FAILED(hr)) return hr;
		if (const auto hr = propsToRead->Add(WPD_OBJECT_PERSISTENT_UNIQUE_ID); FAILED(hr)) return hr;
		if (const auto hr = propsToRead->Add(WPD_OBJECT_FORMAT); FAILED(hr)) return hr;
		if (const auto hr = propsToRead->Add(WPD_OBJECT_CONTENT_TYPE); FAILED(hr)) return hr;
		if (const auto hr = propsToRead->Add(WPD_OBJECT_ORIGINAL_FILE_NAME); FAILED(hr)) return hr;
		if (const auto hr = propsToRead->Add(WPD_OBJECT_SIZE); FAILED(hr)) return hr;

		auto wId = utf8_to_wstring(id);
		CComPtr<IPortableDeviceValues> objectProperties;
		if (const auto hr = props->GetValues(wId.data(), propsToRead, &objectProperties); FAILED(hr)) return hr;

		auto getString = [&](const PROPERTYKEY& key, auto& result) {
			PWSTR strValue;
			if (const auto hr = objectProperties->GetStringValue(key, &strValue); SUCCEEDED(hr)) {
				result = wstring_to_utf8(strValue);
			}
		};

		auto getLong = [&](const PROPERTYKEY& key, auto& result) {
			ULONG value;
			if (const auto hr = objectProperties->GetUnsignedIntegerValue(key, &value); SUCCEEDED(hr)) {
				result = value;
			}
		};

		auto getGuid = [&](const PROPERTYKEY& key, auto& result) {
			GUID guid;
			if (const auto hr = objectProperties->GetGuidValue(key, &guid); SUCCEEDED(hr)) {
				result = guid;
			}
		};

		ObjectProperties result;
		getString(WPD_OBJECT_NAME, result.name);
		getString(WPD_OBJECT_ORIGINAL_FILE_NAME, result.fileName);
		getGuid(WPD_OBJECT_CONTENT_TYPE, result.contentType);
		getGuid(WPD_OBJECT_FORMAT, result.format);
		getLong(WPD_OBJECT_SIZE, result.size);
		return result;
	}

	ExpectedOrHResult<std::vector<ObjectID>> EnumerateContents(CComPtr<IPortableDevice>& device, const ObjectID& id)
	{
		CComPtr<IPortableDeviceContent> content;
		if (const auto hr = device->Content(&content); FAILED(hr)) return hr;

		auto wId = utf8_to_wstring(id);
		CComPtr<IEnumPortableDeviceObjectIDs> enumObjectIDs;
		auto hr = content->EnumObjects(0, wId.data(), nullptr, &enumObjectIDs);
		if (FAILED(hr)) return hr;

		std::vector<ObjectID> results;
		while (hr == S_OK) {
			constexpr auto NUM_OBJECTS_TO_REQUEST = 10;
			PWSTR objectIDs[NUM_OBJECTS_TO_REQUEST];
			DWORD numIds{};
			hr = enumObjectIDs->Next(NUM_OBJECTS_TO_REQUEST, objectIDs, &numIds);
			if (FAILED(hr)) break;

			for (DWORD n = 0; n < numIds; ++n) {
				auto id = objectIDs[n];
				results.push_back(wstring_to_utf8(id));
				CoTaskMemFree(id);
			}
		}
		return results;
	}

	ExpectedOrHResult<size_t> ReadData(CComPtr<IPortableDevice>& device, const ObjectID& id, ReadCallbackFn callback)
	{
		CComPtr<IPortableDeviceContent> content;
		if (const auto hr = device->Content(&content); FAILED(hr)) return hr;

		CComPtr<IPortableDeviceResources> resources;
		if (const auto hr = content->Transfer(&resources); FAILED(hr)) return hr;

		auto wId = utf8_to_wstring(id);
		DWORD optimalTransferSize;
		CComPtr<IStream> stream;
		if (const auto hr = resources->GetStream(wId.data(), WPD_RESOURCE_DEFAULT, STGM_READ, &optimalTransferSize, &stream); FAILED(hr)) return hr;

		size_t totalBytesRead{};
		auto buffer = std::make_unique<char[]>(optimalTransferSize);
		while (true) {
			DWORD bytesRead;
			if (const auto hr = stream->Read(buffer.get(), optimalTransferSize, &bytesRead); FAILED(hr)) return hr;
			if (bytesRead == 0) break;

			totalBytesRead += bytesRead;
			if (!std::invoke(callback, buffer.get(), bytesRead)) break;
		}
		return totalBytesRead;
	}

	ExpectedOrHResult<ObjectID> Lookup(CComPtr<IPortableDevice>& device, const std::vector<std::string>& path)
	{
		ObjectID currentObjectID("DEVICE");
		for (const auto& piece : path)
		{
			auto contents = EnumerateContents(device, currentObjectID);
			if (!contents) return contents.GetResult();
			auto nextObjectID = std::find_if(contents->begin(), contents->end(), [&](const auto& id) {
				auto props = ReadProperties(device, id);
				return props && props->name == piece;
			});
			if (nextObjectID == contents->end()) return ObjectID{};

			currentObjectID = *nextObjectID;
		}
		return currentObjectID;
	}
}

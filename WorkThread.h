#pragma once

#include <QThread>
#include "MTP.h"
#include "Config.h"

struct BackupLocation
{
	mtp::ObjectID objectId;
	std::string where;
};
using BackupLocations = std::vector<BackupLocation>;

class WorkThread : public QThread
{
	Q_OBJECT

public:
	WorkThread(QObject* parent, CComPtr<IPortableDevice> activeDevice, BackupLocations locations);
	virtual ~WorkThread();
	
	struct Impl;

signals:
	void numbersUpdated(const NumbersAvailable&);
	void numbersComplete(const NumbersAvailable&);
	void itemsUpdated(const NumbersAvailable&, const ItemsUpdate&);
	void finished(const ItemsUpdate&, const std::vector<FailedItem>&);

public:
	void run() override;

private:
	std::unique_ptr<Impl> impl;
};


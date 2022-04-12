/*-
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Copyright (c) 2022 Rink Springer <rink@rink.nu>
 * For conditions of distribution and use, see LICENSE file
 */
#include "WorkingDialog.h"
#include "MTP.h"
#include "WorkThread.h"
#include <QMessageBox>

WorkingDialog::WorkingDialog(QWidget* parent, CComPtr<IPortableDevice>& activeDevice, BackupLocations locations)
    : QDialog(parent)
    , activeDevice(activeDevice)
{
    ui.setupUi(this);

    ui.progressBar->setMinimum(0);
    ui.progressBar->setMaximum(0);
    ui.progressBar->setValue(0);

    ui.status->setText("Determining number of items to copy");

    workThread = std::make_unique<WorkThread>(this, activeDevice, std::move(locations));
	connect(workThread.get(), &WorkThread::numbersUpdated, this, &WorkingDialog::OnNumbersUpdated);
	connect(workThread.get(), &WorkThread::numbersComplete, this, &WorkingDialog::OnNumbersComplete);
	connect(workThread.get(), &WorkThread::itemsUpdated, this, &WorkingDialog::OnItemsUpdated);
	connect(workThread.get(), &WorkThread::finished, this, &WorkingDialog::OnFinished);
    workThread->start();
}

WorkingDialog::~WorkingDialog() = default;

void WorkingDialog::OnNumbersUpdated(const NumbersAvailable& na)
{
	auto s(QString("Scanning: %1 items totalling %2 KB").arg(na.totalNumberOfItems).arg(na.totalNumberOfBytes / 1024));
	ui.status->setText(s);
}

void WorkingDialog::OnNumbersComplete(const NumbersAvailable& na)
{
	auto s(QString("Copying: %1 items totalling %2 KB").arg(na.totalNumberOfItems).arg(na.totalNumberOfBytes / 1024));
	ui.status->setText(s);
    ui.progressBar->setMaximum(na.totalNumberOfBytes / 1024);
}

void WorkingDialog::OnItemsUpdated(const NumbersAvailable& na, const ItemsUpdate& iu)
{
    const auto total = (iu.bytesRead + iu.bytesSkipped) / 1024;
    ui.progressBar->setValue(total);
    auto s(QString("Copying: %1 of %2 items copied, %3 skipped, %4 failured").arg(iu.itemsTransferredSuccessfully).arg(na.totalNumberOfItems).arg(iu.itemsTransferredSkipped).arg(iu.itemsTransferredFailures));
	ui.status->setText(s);
}

void WorkingDialog::OnFinished(const ItemsUpdate& iu, const std::vector<FailedItem>& failedItems)
{
    if (!failedItems.empty()) {
		QMessageBox::warning(this, "Warning", QString("Unable to transfer %1 item(s)").arg(failedItems.size()));
    }
    accept();
}

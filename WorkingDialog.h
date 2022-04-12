/*-
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Copyright (c) 2022 Rink Springer <rink@rink.nu>
 * For conditions of distribution and use, see LICENSE file
 */
#pragma once

#include <QtWidgets/QWidget>
#include "ui_Working.h"
#include "MTP.h"
#include "WorkThread.h"

struct NumbersAvailable;
struct Configuration;

class WorkingDialog : public QDialog
{
    Q_OBJECT

public:

private:
    Ui::Working ui;
    CComPtr<IPortableDevice> activeDevice;
    std::unique_ptr<WorkThread> workThread;

    void OnNumbersUpdated(const NumbersAvailable&);
    void OnNumbersComplete(const NumbersAvailable&);
    void OnItemsUpdated(const NumbersAvailable&, const ItemsUpdate&);
    void OnFinished(const ItemsUpdate& iu, const std::vector<FailedItem>& failedItems);

public:
    WorkingDialog(QWidget* parent, CComPtr<IPortableDevice>& activeDevice, BackupLocations);
    virtual ~WorkingDialog();
};

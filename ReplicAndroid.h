/*-
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Copyright (c) 2022 Rink Springer <rink@rink.nu>
 * For conditions of distribution and use, see LICENSE file
 */
#pragma once

#include <QtWidgets/QWidget>
#include "ui_ReplicAndroid.h"

class QStandardItemModel;

class ReplicAndroid : public QWidget
{
    Q_OBJECT

private:
    Ui::ReplicAndroidClass ui;
    std::unique_ptr<QStandardItemModel> whatModel;
    void OnDeviceOpenedOrClosed();
    void UpdateWhatModel();
    void UpdateWherePath();

private slots:
    void OnConnectClicked();
    void OnWhereClicked();
    void OnWhatAddClicked();
    void OnWhatRemoveClicked();
    void OnWhatSelectionChanged();
    void OnStartClicked();

public:
    ReplicAndroid(QWidget *parent = Q_NULLPTR);
    virtual ~ReplicAndroid();
};

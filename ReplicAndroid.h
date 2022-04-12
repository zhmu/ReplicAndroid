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

/*-
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Copyright (c) 2022 Rink Springer <rink@rink.nu>
 * For conditions of distribution and use, see LICENSE file
 */
#include "ReplicAndroid.h"
#include "MTP.h"
#include "BrowseDialog.h"
#include "WorkingDialog.h"
#include "Config.h"
#include "WorkThread.h"
#include <comdef.h>
#include <QFileDialog>
#include <QMessageBox>
#include <QStandardItemModel>

namespace
{
    CComPtr<IPortableDevice> activeDevice;
    Configuration config;

    void UpdateDeviceList(Ui::ReplicAndroidClass& ui)
    {
        auto devices = mtp::EnumeratePortableDevices();
        if (devices)
        {
            ui.cmbDevices->clear();
            for (const auto& device : *devices) {
                auto extractStringOrNone = [](const auto& s) -> QString
                {
                    if (s) return s->c_str();
                    return "<none>";
                };
                ui.cmbDevices->addItem(QString("%1 - %2 %3").arg(extractStringOrNone(device.friendlyName), extractStringOrNone(device.manufacturer), extractStringOrNone(device.description)), QString(device.id.c_str()));
            }
            ui.btnConnect->setEnabled(!devices->empty());
        }
    }

    void ReadSettings()
    {
		// XXX
        config.what.clear();
        {
            WhatItem wi;
            wi.path.push_back("Internal storage");
            wi.path.push_back("DCIM");
            wi.path.push_back("Camera");
            config.what.push_back(std::move(wi));
        }
        config.where = "C:/Temp";
    }

    void ReportError(QWidget* parent, HRESULT hr)
    {
        auto e = _com_error(hr);
        QString s = QString("Error: %1 (%2)").arg(e.ErrorMessage(), hr);
        QMessageBox::critical(parent, "Error occured", s);
    }
}

void ReplicAndroid::OnDeviceOpenedOrClosed()
{
	const bool isDeviceConnected = activeDevice;

	if (isDeviceConnected) {
		ui.btnConnect->setText("&Disconnect");
		ReadSettings();
		UpdateWhatModel();
        UpdateWherePath();
	} else {
		ui.btnConnect->setText("&Connect");
		UpdateDeviceList(ui);
	}
	ui.cmbDevices->setEnabled(!isDeviceConnected);
	ui.btnWhatAdd->setEnabled(isDeviceConnected);
	ui.btnWhatRemove->setEnabled(false);
	ui.btnWhereBrowse->setEnabled(isDeviceConnected);
	ui.btnStart->setEnabled(isDeviceConnected);
}

void ReplicAndroid::OnConnectClicked()
{
    if (activeDevice) {
        activeDevice.Release();
    } else {
        const auto deviceId = ui.cmbDevices->currentData().toString();
        auto device = mtp::OpenDevice(deviceId.toStdString());
        if (device) activeDevice = *device;
    }
	OnDeviceOpenedOrClosed();
}

void ReplicAndroid::OnWhatAddClicked()
{
    BrowseDialog dlg(activeDevice, this);
    if (dlg.exec() != QDialog::Accepted) return;

    WhatItem wi;
    for (const auto& piece : dlg.GetPath()) {
        wi.path.push_back(piece.name);
    }
    config.what.push_back(std::move(wi));
    UpdateWhatModel();
}

void ReplicAndroid::OnWhatRemoveClicked()
{
    auto index = ui.lvWhat->currentIndex();
    auto whatIndex = whatModel->data(index, Qt::UserRole).toInt();
    config.what.erase(config.what.begin() + whatIndex);
    UpdateWhatModel();
}

void ReplicAndroid::OnWhatSelectionChanged()
{
    ui.btnWhatRemove->setEnabled(true);
}

void ReplicAndroid::OnWhereClicked()
{
    QFileDialog dlg(this, {}, QString(config.where.c_str()));
    dlg.setFileMode(QFileDialog::Directory);
    dlg.setOption(QFileDialog::ShowDirsOnly);
    if (dlg.exec() != QDialog::Accepted) return;

    config.where = dlg.directory().path().toStdString();
    UpdateWherePath();
}

void ReplicAndroid::UpdateWhatModel()
{
	whatModel->clear();
    for(size_t n = 0; n < config.what.size(); ++n)
	{
		QString s;
		for (const auto& piece : config.what[n].path)
        {
			s += "/";
			s += piece.c_str();
		}
        auto item = new QStandardItem(s);
       item->setData(n, Qt::UserRole);
       whatModel->appendRow(item);
    }
    ui.btnWhatRemove->setEnabled(false);
}

void ReplicAndroid::UpdateWherePath()
{
    ui.edtStorePath->setText(config.where.c_str());
}

void ReplicAndroid::OnStartClicked()
{
    BackupLocations locations;

    for (const auto& what : config.what) {
        auto objectId = mtp::Lookup(activeDevice, what.path);
        if (!objectId)
        {
            ReportError(this, objectId.GetResult());
            return;
        }
        if (objectId->empty())
        {
            QString s;
            for (const auto& piece : what.path) { s += "/"; s += piece.c_str(); }
            QMessageBox::critical(this, "Error", QString("Unable to locate %1 on device, aborting backup").arg(s));
            return;
        }

        std::string destLocation;
        for (const auto& piece : what.path) {
            destLocation += piece;
        }
        destLocation.erase(std::remove_if(destLocation.begin(), destLocation.end(), [](const auto v) {
            const auto ch = static_cast<unsigned char>(v);
            return !std::isalnum(ch);
		}), destLocation.end());

        std::string destPath = config.where + '/' + destLocation;
        locations.push_back({ *objectId, destPath });
    }

	WorkingDialog dlg(this, activeDevice, std::move(locations));
    dlg.exec();
}

ReplicAndroid::ReplicAndroid(QWidget *parent)
    : QWidget(parent)
{
    ui.setupUi(this);
    whatModel = std::make_unique<QStandardItemModel>(this);
    ui.lvWhat->setModel(whatModel.get());

    connect(ui.btnConnect, &QPushButton::clicked, this, &ReplicAndroid::OnConnectClicked);
    connect(ui.btnWhatAdd, &QPushButton::clicked, this, &ReplicAndroid::OnWhatAddClicked);
    connect(ui.btnWhatRemove, &QPushButton::clicked, this, &ReplicAndroid::OnWhatRemoveClicked);
    connect(ui.lvWhat->selectionModel(), &QItemSelectionModel::currentChanged, this, &ReplicAndroid::OnWhatSelectionChanged);
    connect(ui.btnWhereBrowse, &QPushButton::clicked, this, &ReplicAndroid::OnWhereClicked);
    connect(ui.btnStart, &QPushButton::clicked, this, &ReplicAndroid::OnStartClicked);

    ui.lvWhat->setEditTriggers(QAbstractItemView::NoEditTriggers);

    UpdateDeviceList(ui);
	OnDeviceOpenedOrClosed();
}

ReplicAndroid::~ReplicAndroid() = default;

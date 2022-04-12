#pragma once

#include <QtWidgets/QWidget>
#include <memory>
#include "ui_Browse.h"
#include "MTP.h"

class QStandardItemModel;

class BrowseDialog : public QDialog
{
    Q_OBJECT

public:
    struct PathItem {
        mtp::ObjectID id;
        std::string name;
    };

private:
    Ui::Browse ui;
    CComPtr<IPortableDevice> activeDevice;
    std::vector<PathItem> path;
    std::unique_ptr<QStandardItemModel> model;

    void UpdateModel();

private slots:
    void OnOkClicked();
    void OnCancelClicked();
    void OnListViewDoubleClicked();

public:
    BrowseDialog(CComPtr<IPortableDevice>&, QWidget *parent = Q_NULLPTR);
    virtual ~BrowseDialog();

    const auto& GetPath() const { return path;  }
};

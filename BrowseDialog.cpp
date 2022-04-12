#include "BrowseDialog.h"
#include "MTP.h"
#include <QStandardItemModel>

BrowseDialog::BrowseDialog(CComPtr<IPortableDevice>& activeDevice, QWidget* parent)
    : QDialog(parent)
    , activeDevice(activeDevice)
{
    ui.setupUi(this);
    connect(ui.okButton, &QPushButton::clicked, this, &BrowseDialog::OnOkClicked);
    connect(ui.cancelButton, &QPushButton::clicked, this, &BrowseDialog::OnCancelClicked);
    connect(ui.lvItems, &QListView::doubleClicked, this, &BrowseDialog::OnListViewDoubleClicked);

    ui.lvItems->setEditTriggers(QAbstractItemView::NoEditTriggers);
    model = std::make_unique<QStandardItemModel>(this);
    ui.lvItems->setModel(model.get());

    UpdateModel();
}

BrowseDialog::~BrowseDialog() = default;

void BrowseDialog::UpdateModel()
{
    const auto objectId = [&]() -> std::string {
        if (path.empty()) return "DEVICE";
        return path.back().id;
    }();
    auto contents = mtp::EnumerateContents(activeDevice, objectId);
    if (!contents) return;

    auto dirIcon = style()->standardIcon(QStyle::SP_DirIcon);
    auto parentIcon = style()->standardIcon(QStyle::SP_FileDialogToParent);

    model->clear();
    if (!path.empty()) {
        model->appendRow(new QStandardItem(parentIcon, "<back>"));
    }
    for (const auto& id : *contents)
    {
       auto props = mtp::ReadProperties(activeDevice, id);
       if (!props) continue;
       if (!props->name) continue;

       auto item = new QStandardItem(dirIcon, QString(props->name.value().c_str()));
       item->setData(QString(id.c_str()), Qt::UserRole);
       model->appendRow(item);
    }
}

void BrowseDialog::OnListViewDoubleClicked()
{
    auto index = ui.lvItems->currentIndex();
    auto userData = model->data(index, Qt::UserRole);
    if (!userData.isNull())
    {
        auto name = model->data(index, Qt::DisplayRole).toString().toStdString();
        auto id = userData.toString().toStdString();
        path.push_back({ id, name });
    }
    else {
        path.pop_back();
    }
    UpdateModel();
}

void BrowseDialog::OnOkClicked()
{
    accept();
}

void BrowseDialog::OnCancelClicked()
{
    reject();
}

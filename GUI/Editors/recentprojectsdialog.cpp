// recentprojectsdialog.cpp
#include "recentprojectsdialog.h"
#include <QFileInfo>

RecentProjectsDialog::RecentProjectsDialog(QWidget *parent)
    : QDialog(parent), selectedFile("")
{
    setWindowTitle("Recent Projects");
    setMinimumSize(400, 300);

    listWidget = new QListWidget(this);
    connect(listWidget, &QListWidget::itemDoubleClicked, this, &RecentProjectsDialog::onItemDoubleClicked);

    openButton = new QPushButton("Open", this);
    cancelButton = new QPushButton("Cancel", this);

    connect(openButton, &QPushButton::clicked, this, &RecentProjectsDialog::onOpenClicked);
    connect(cancelButton, &QPushButton::clicked, this, &RecentProjectsDialog::onCancelClicked);

    QHBoxLayout *buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch();
    buttonLayout->addWidget(openButton);
    buttonLayout->addWidget(cancelButton);

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->addWidget(listWidget);
    mainLayout->addLayout(buttonLayout);

    setLayout(mainLayout);
}

void RecentProjectsDialog::setRecentProjects(const QStringList &projects)
{
    listWidget->clear();
    for (const QString &project : projects) {
        QFileInfo fileInfo(project);
        QListWidgetItem *item = new QListWidgetItem(fileInfo.fileName());
        item->setData(Qt::UserRole, project);
        item->setToolTip(project);
        listWidget->addItem(item);
    }
}

void RecentProjectsDialog::onItemDoubleClicked(QListWidgetItem *item)
{
    selectedFile = item->data(Qt::UserRole).toString();
    accept();
}

void RecentProjectsDialog::onOpenClicked()
{
    QListWidgetItem *currentItem = listWidget->currentItem();
    if (currentItem) {
        selectedFile = currentItem->data(Qt::UserRole).toString();
        accept();
    }
}

void RecentProjectsDialog::onCancelClicked()
{
    reject();
}

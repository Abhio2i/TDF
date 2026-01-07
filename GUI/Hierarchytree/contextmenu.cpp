
#include "contextmenu.h"
#include "GUI/Hierarchytree/additemdialog.h"
#include "core/Hierarchy/hierarchy.h"

#include <QInputDialog>
#include <QDebug>
#include <QAction>
#include <QLineEdit>

/* Initialize context menu */
ContextMenu::ContextMenu(QWidget *parent)
    : QMenu(parent)
{
}
/* Setup context menu for item */
void ContextMenu::setupMenu(QTreeWidgetItem *item)
{
    if (!item) return;
    clear();
    QVariantMap storedData = item->data(0, Qt::UserRole).toMap();
    QString ID = storedData["ID"].toString();
    QString parentID = storedData["parentId"].toString();
    QString name = storedData["name"].toString();
    QString type;
    QString specificType;
    if (storedData["type"].type() == QVariant::Map) {
        QVariantMap typeData = storedData["type"].toMap();
        if (typeData.contains("type") && typeData["type"].toString() == "option") {
            type = "profile";
            specificType = typeData.value("value", "").toString();
        } else {
            qWarning() << "Invalid nested type structure in item data:" << storedData["type"];
            return;
        }
    } else {
        type = storedData["type"].toString();
    }
    if (type == "subcomponent") {
        setupSubComponentMenu(storedData);
    } else if (type == "profile") {
        setupProfileMenu(storedData);
    } else if (type == "folder") {
        setupFolderMenu(storedData);
    } else if (type == "entity") {
        setupEntityMenu(storedData);
    } else if (type == "component") {
        setupComponentMenu(storedData);
    }
}
void ContextMenu::setupSubComponentMenu(const QVariantMap &data)
{
    QString ID = data["ID"].toString();
    QString parentID = data["parentId"].toString();
    QString name = data["name"].toString();
    QString componentType = data["componentType"].toString();
    QAction *removeSubComponent = addAction("Remove");
    QAction *rename = addAction("Rename");
    connect(removeSubComponent, &QAction::triggered, this, [=]() {
        emit removeSubComponentRequested(parentID, ID, name);
    });
    connect(rename, &QAction::triggered, this, [=]() mutable {
        bool ok;
        QString newName = QInputDialog::getText(this, "Rename", "Enter New Name:",
                                                QLineEdit::Normal, name, &ok);
        if (ok && !newName.trimmed().isEmpty()) {
            QVariantMap modifiedData = data;
            modifiedData["name"] = newName;
            emit renameItemRequested(modifiedData);
        }
    });
}

void ContextMenu::setupProfileMenu(const QVariantMap &data)
{
    QString ID = data["ID"].toString();
    QString name = data["name"].toString();
    QString specificType = data["type"].toMap()["value"].toString();
    QAction *rename = addAction("Rename");
    QAction *paste = addAction("Paste");
    QAction *addFolder = addAction("Add Folder");
    QAction *addEntity = addAction("Add Entity");
    QAction *addProfile = addAction("Add Profile");
    QAction *deleteProfile = addAction("Delete Profile");
    connect(addFolder, &QAction::triggered, this, [=]() {
        bool ok;
        QString folderName = QInputDialog::getText(this, "Add Folder", "Enter Folder Name:",
                                                   QLineEdit::Normal, "New Folder", &ok);
        if (ok && !folderName.trimmed().isEmpty()) {
            if (!folderName.isEmpty()) {
                folderName[0] = folderName[0].toUpper();
            }
            emit addFolderRequested(ID, folderName, true, QVariantMap());
        }
    });
    connect(addEntity, &QAction::triggered, this, [=]() {
        bool isSensorProfile = (specificType.toLower() == "sensor");
        AddItemDialog dialog(AddItemDialog::EntityType,
                             specificType,
                             AddItemDialog::NormalMode,
                             m_hierarchy,
                             this);
        if (dialog.exec() == QDialog::Accepted && !dialog.getName().isEmpty()) {
            bool isProfileParent = true;
            if (isSensorProfile) {
                emit addEntityRequested(ID, dialog.getName(),
                                        isProfileParent, dialog.getComponents(), &dialog);
            } else {

                for (int i = 0; i < dialog.getNumber(); ++i) {
                    emit addEntityRequested(ID, dialog.getName() + QString::number(i),
                                            isProfileParent, dialog.getComponents(), &dialog);
                }
            }
        }
    });
    connect(addProfile, &QAction::triggered, this, [=]() {
        bool ok;
        QString profileName = QInputDialog::getText(this, "Add Profile", "Enter Profile Name:",
                                                    QLineEdit::Normal, "New Profile", &ok);
        if (ok && !profileName.trimmed().isEmpty()) {
            emit addProfileRequested(profileName);
        }
    });
    connect(deleteProfile, &QAction::triggered, this, [=]() {
        emit removeProfileRequested(ID);
    });
    connect(paste, &QAction::triggered, this, [=]() {
        emit pasteItemRequested(data);
    });
    connect(rename, &QAction::triggered, this, [=]() mutable {
        bool ok;
        QString newName = QInputDialog::getText(this, "Rename", "Enter New Name:",
                                                QLineEdit::Normal, name, &ok);
        if (ok && !newName.trimmed().isEmpty()) {
            QVariantMap modifiedData = data;
            modifiedData["name"] = newName;
            if (modifiedData["type"].type() == QVariant::Map) {
                QVariantMap typeData = modifiedData["type"].toMap();
                typeData["value"] = newName;
                modifiedData["type"] = typeData;
            }
            emit renameItemRequested(modifiedData);
        }
    });
}
void ContextMenu::setupFolderMenu(const QVariantMap &data)
{
    QString ID = data["ID"].toString();
    QString parentID = data["parentId"].toString();
    QString name = data["name"].toString();
    QAction *rename = addAction("Rename");
    QAction *paste = addAction("Paste");
    QAction *addFolder = addAction("Add Folder");
    QAction *addEntity = addAction("Add Entity");
    QAction *deleteFolder = addAction("Delete Folder");
    connect(addFolder, &QAction::triggered, this, [=]() {
        bool ok;
        QString folderName = QInputDialog::getText(this, "Add Folder", "Enter Folder Name:",
                                                   QLineEdit::Normal, "New Folder", &ok);
        if (ok && !folderName.trimmed().isEmpty()) {
            if (!folderName.isEmpty()) {
                folderName[0] = folderName[0].toUpper();
            }
            emit addFolderRequested(ID, folderName, false, QVariantMap());
        }
    });
    connect(addEntity, &QAction::triggered, this, [=]() {
        AddItemDialog dialog(AddItemDialog::EntityType, "",
                             AddItemDialog::NormalMode, m_hierarchy, this);
        if (dialog.exec() == QDialog::Accepted && !dialog.getName().isEmpty()) {
            for (int i = 0; i < dialog.getNumber(); ++i) {
                emit addEntityRequested(ID, dialog.getName() + QString::number(i),
                                        false, dialog.getComponents(), &dialog);
            }
        }
    });
    connect(deleteFolder, &QAction::triggered, this, [=]() {
        emit removeFolderRequested(parentID, ID, false);
    });
    connect(paste, &QAction::triggered, this, [=]() {
        emit pasteItemRequested(data);
    });
    connect(rename, &QAction::triggered, this, [=]() mutable {
        bool ok;
        QString newName = QInputDialog::getText(this, "Rename", "Enter New Name:",                                                QLineEdit::Normal, name, &ok);
        if (ok && !newName.trimmed().isEmpty()) {
            QVariantMap modifiedData = data;
            modifiedData["name"] = newName;
            emit renameItemRequested(modifiedData);
        }
    });
}
void ContextMenu::setupEntityMenu(const QVariantMap &data)
{
    QString ID = data["ID"].toString();
    QString parentID = data["parentId"].toString();
    QString name = data["name"].toString();
    QAction *rename = addAction("Rename");
    QAction *copy = addAction("Copy");
    QAction *deleteEntity = addAction("Delete Entity");
    connect(deleteEntity, &QAction::triggered, this, [=]() {
        emit removeEntityRequested(parentID, ID, false);
    });
    connect(copy, &QAction::triggered, this, [=]() {
        emit copyItemRequested(data);
    });
    connect(rename, &QAction::triggered, this, [=]() mutable {
        bool ok;
        QString newName = QInputDialog::getText(this, "Rename", "Enter New Name:",
                                                QLineEdit::Normal, name, &ok);
        if (ok && !newName.trimmed().isEmpty()) {
            QVariantMap modifiedData = data;
            modifiedData["name"] = newName;
            emit renameItemRequested(modifiedData);
        }
    });
}

void ContextMenu::setupComponentMenu(const QVariantMap &data)
{
    QString ID = data["ID"].toString();
    QString parentID = data["parentId"].toString();
    QString name = data["name"].toString();
    QStringList specialComponents = {"radios", "sensors", "iffs"};
    if (specialComponents.contains(name.toLower())) {
        QAction *addComponent = addAction("Add");
        QAction *removeComponent = addAction("Remove");
        connect(addComponent, &QAction::triggered, this, [=]() {
            QString componentType = name.toLower();
            AddItemDialog::DialogMode mode = AddItemDialog::NormalMode;
            if (componentType == "sensors") {
                mode = AddItemDialog::ComponentSensorMode;
            } else if (componentType == "iffs") {
                mode = AddItemDialog::ComponentIFFMode;
            } else if (componentType == "radios") {
                mode = AddItemDialog::ComponentRadioMode;
            }
            AddItemDialog dialog(AddItemDialog::EntityType,
                                 componentType,
                                 mode,
                                 m_hierarchy,
                                 this);

            if (dialog.exec() == QDialog::Accepted && !dialog.getName().isEmpty()) {
                QString profileName = dialog.getProfileName();
                QString profileId = dialog.getProfileId();
                QString sensorType = (componentType == "sensors") ?
                                         dialog.getSensorType() : "";
                emit addComponentRequested(ID, componentType, dialog.getName(),
                                           sensorType, profileId);
            }
        });
        connect(removeComponent, &QAction::triggered, this, [=]() {
            emit removeComponentRequested(parentID, name.toLower());
        });
    } else {
        QAction *removeComponent = addAction("Remove");
        connect(removeComponent, &QAction::triggered, this, [=]() {
            emit removeComponentRequested(parentID, name);
        });
    }
}

/* ========================================================================= */
/* File: contextmenu.cpp                                                    */
/* Purpose: Implements context menu for hierarchy tree items                */
/* ========================================================================= */

#include "contextmenu.h"                           // For context menu class
#include "additemdialog.h"                         // For add item dialog
#include <QInputDialog>                            // For input dialog
#include <QDebug>                                  // For debug output
#include <QAction>                                 // For action handling

// %%% Constructor %%%
/* Initialize context menu */
ContextMenu::ContextMenu(QWidget *parent)
    : QMenu(parent)
{
    // No additional initialization needed
}

/* Setup context menu for item */
void ContextMenu::setupMenu(QTreeWidgetItem *item)
{
    // Return if item is null
    if (!item) return;

    // Clear existing menu actions
    clear();
    // Retrieve item data
    QVariantMap storedData = item->data(0, Qt::UserRole).toMap();
    QString ID = storedData["ID"].toString();
    QString parentID = storedData["parentId"].toString();
    QString name = storedData["name"].toString();
    QString type;
    QString specificType;

    // Determine item type
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

    // Log menu setup details
    qDebug() << "Setting up context menu for: name=" << name << "type=" << type << "specificType=" << specificType << "ID=" << ID;

    // %%% Profile Menu %%%
    if (type == "profile") {
        // Add profile actions
        QAction *rename = addAction("Rename");
        QAction *paste = addAction("Paste");
        QAction *addFolder = addAction("Add Folder");
        QAction *addEntity = addAction("Add Entity");
        QAction *addProfile = addAction("Add Profile");
        QAction *deleteProfile = addAction("Delete Profile");

        // Connect add folder action
        connect(addFolder, &QAction::triggered, this, [=]() {
            AddItemDialog dialog(AddItemDialog::Folder, "", this);
            if (dialog.exec() == QDialog::Accepted && !dialog.getName().isEmpty()) {
                emit addFolderRequested(ID, dialog.getName(), true, dialog.getComponents());
            }
        });

        // Connect add entity action
        connect(addEntity, &QAction::triggered, this, [=]() {
            AddItemDialog dialog(AddItemDialog::EntityType, specificType, this);
            if (dialog.exec() == QDialog::Accepted && !dialog.getName().isEmpty()) {
                bool isProfileParent = true;
                for (int i = 0; i < dialog.getNumber(); i++) {
                    emit addEntityRequested(ID, dialog.getName()+ QString::number(i), isProfileParent, dialog.getComponents());
                }
            }
        });

        // Connect add profile action
        connect(addProfile, &QAction::triggered, this, [=]() {
            bool ok;
            QString profileName = QInputDialog::getText(this, "Add Profile", "Enter Profile Name:",
                                                        QLineEdit::Normal, "New Profile", &ok);
            if (ok && !profileName.trimmed().isEmpty()) {
                emit addProfileRequested(profileName);
            }
        });

        // Connect delete profile action
        connect(deleteProfile, &QAction::triggered, this, [=]() {
            emit removeProfileRequested(ID);
        });

        // Connect paste action
        connect(paste, &QAction::triggered, this, [=]() {
            emit pasteItemRequested(storedData);
        });

        // Connect rename action
        connect(rename, &QAction::triggered, this, [=]() mutable {
            bool ok;
            QString newName = QInputDialog::getText(this, "Rename", "Enter New Name:",
                                                    QLineEdit::Normal, name, &ok);
            if (ok && !newName.trimmed().isEmpty()) {
                storedData["name"] = newName;
                if (storedData["type"].type() == QVariant::Map) {
                    QVariantMap typeData = storedData["type"].toMap();
                    typeData["value"] = newName;
                    storedData["type"] = typeData;
                }
                emit renameItemRequested(storedData);
            }
        });

        // %%% Folder Menu %%%
    } else if (type == "folder") {
        // Add folder actions
        QAction *rename = addAction("Rename");
        QAction *paste = addAction("Paste");
        QAction *addFolder = addAction("Add Folder");
        QAction *addEntity = addAction("Add Entity");
        QAction *deleteFolder = addAction("Delete Folder");

        // Connect add folder action
        connect(addFolder, &QAction::triggered, this, [=]() {
            AddItemDialog dialog(AddItemDialog::Folder, "", this);
            if (dialog.exec() == QDialog::Accepted && !dialog.getName().isEmpty()) {
                emit addFolderRequested(ID, dialog.getName(), false, dialog.getComponents());
            }
        });

        // Connect add entity action
        connect(addEntity, &QAction::triggered, this, [=]() {
            AddItemDialog dialog(AddItemDialog::EntityType, "", this);
            if (dialog.exec() == QDialog::Accepted && !dialog.getName().isEmpty()) {
                for (int i = 0; i < dialog.getNumber(); i++) {
                    emit addEntityRequested(ID, dialog.getName()+ QString::number(i), false, dialog.getComponents());
                }
            }
        });

        // Connect delete folder action
        connect(deleteFolder, &QAction::triggered, this, [=]() {
            emit removeFolderRequested(parentID, ID, false);
        });

        // Connect paste action
        connect(paste, &QAction::triggered, this, [=]() {
            emit pasteItemRequested(storedData);
        });

        // Connect rename action
        connect(rename, &QAction::triggered, this, [=]() mutable {
            bool ok;
            QString newName = QInputDialog::getText(this, "Rename", "Enter New Name:",
                                                    QLineEdit::Normal, name, &ok);
            if (ok && !newName.trimmed().isEmpty()) {
                storedData["name"] = newName;
                emit renameItemRequested(storedData);
            }
        });

        // %%% Entity Menu %%%
    } else if (type == "entity") {
        // Add entity actions
        QAction *rename = addAction("Rename");
        QAction *copy = addAction("Copy");
        QAction *deleteEntity = addAction("Delete Entity");

        // Connect delete entity action
        connect(deleteEntity, &QAction::triggered, this, [=]() {
            emit removeEntityRequested(parentID, ID, false);
        });

        // Connect copy action
        connect(copy, &QAction::triggered, this, [=]() {
            emit copyItemRequested(storedData);
        });

        // Connect rename action
        connect(rename, &QAction::triggered, this, [=]() mutable {
            bool ok;
            QString newName = QInputDialog::getText(this, "Rename", "Enter New Name:",
                                                    QLineEdit::Normal, name, &ok);
            if (ok && !newName.trimmed().isEmpty()) {
                storedData["name"] = newName;
                emit renameItemRequested(storedData);
            }
        });

        // %%% Component Menu %%%
    } else if (type == "component") {
        // Define special components
        QStringList specialComponents = {"radios", "sensors", "iff"};
        if (specialComponents.contains(name.toLower())) {
            // Add special component actions
            QAction *addComponent = addAction("Add");
            QAction *removeComponent = addAction("Remove");

            // Connect add component action
            connect(addComponent, &QAction::triggered, this, [=]() {
                bool ok;
                QString componentName = QInputDialog::getText(this, "Add Component", "Enter Component Name:",
                                                              QLineEdit::Normal, name, &ok);
                if (ok && !componentName.trimmed().isEmpty()) {
                    emit addComponentRequested(parentID, name.toLower(), componentName);
                }
            });

            // Connect remove component action
            connect(removeComponent, &QAction::triggered, this, [=]() {
                emit removeComponentRequested(parentID, name.toLower());
            });
        } else {
            // Add standard component action
            QAction *removeComponent = addAction("Remove");
            connect(removeComponent, &QAction::triggered, this, [=]() {
                emit removeComponentRequested(parentID, name);
            });
        }
    }
}

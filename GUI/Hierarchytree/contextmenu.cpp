#include "contextmenu.h"
#include "additemdialog.h"
#include <QInputDialog>
#include <QDebug>
#include <QAction>

ContextMenu::ContextMenu(QWidget *parent)
    : QMenu(parent)
{
}

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

    // Check if type is a nested map (for profiles) or a string (for others)
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

    qDebug() << "Setting up context menu for: name=" << name << "type=" << type << "specificType=" << specificType << "ID=" << ID;

    if (type == "profile") {
        QAction *rename = addAction("Rename");
        QAction *paste = addAction("Paste");
        QAction *addFolder = addAction("Add Folder");
        QAction *addEntity = addAction("Add Entity");
        QAction *addProfile = addAction("Add Profile");
        QAction *deleteProfile = addAction("Delete Profile");

        connect(addFolder, &QAction::triggered, this, [=]() {
            AddItemDialog dialog(AddItemDialog::Folder, "", this);
            if (dialog.exec() == QDialog::Accepted && !dialog.getName().isEmpty()) {
                emit addFolderRequested(ID, dialog.getName(), true, dialog.getComponents());
            }
        });

        connect(addEntity, &QAction::triggered, this, [=]() {
            AddItemDialog dialog(AddItemDialog::EntityType, specificType, this);
            if (dialog.exec() == QDialog::Accepted && !dialog.getName().isEmpty()) {
                bool isProfileParent = true;
                for (int i = 0; i < dialog.getNumber(); i++) {
                    emit addEntityRequested(ID, dialog.getName()+ QString::number(i), isProfileParent, dialog.getComponents());
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
            emit pasteItemRequested(storedData);
        });

        connect(rename, &QAction::triggered, this, [=]() mutable {
            bool ok;
            QString newName = QInputDialog::getText(this, "Rename", "Enter New Name:",
                                                    QLineEdit::Normal, name, &ok);
            if (ok && !newName.trimmed().isEmpty()) {
                storedData["name"] = newName;
                // Update the nested type value if it's a profile
                if (storedData["type"].type() == QVariant::Map) {
                    QVariantMap typeData = storedData["type"].toMap();
                    typeData["value"] = newName;
                    storedData["type"] = typeData;
                }
                emit renameItemRequested(storedData);
            }
        });

    } else if (type == "folder") {
        QAction *rename = addAction("Rename");
        QAction *paste = addAction("Paste");
        QAction *addFolder = addAction("Add Folder");
        QAction *addEntity = addAction("Add Entity");
        QAction *deleteFolder = addAction("Delete Folder");

        connect(addFolder, &QAction::triggered, this, [=]() {
            AddItemDialog dialog(AddItemDialog::Folder, "", this);
            if (dialog.exec() == QDialog::Accepted && !dialog.getName().isEmpty()) {
                emit addFolderRequested(ID, dialog.getName(), false, dialog.getComponents());
            }
        });

        connect(addEntity, &QAction::triggered, this, [=]() {
            AddItemDialog dialog(AddItemDialog::EntityType, "", this);
            if (dialog.exec() == QDialog::Accepted && !dialog.getName().isEmpty()) {
                // Emit signal for each entity
                for (int i = 0; i < dialog.getNumber(); i++) {
                    emit addEntityRequested(ID, dialog.getName()+ QString::number(i), false, dialog.getComponents());
                }
            }
        });

        connect(deleteFolder, &QAction::triggered, this, [=]() {
            emit removeFolderRequested(parentID, ID, false);
        });

        connect(paste, &QAction::triggered, this, [=]() {
            emit pasteItemRequested(storedData);
        });

        connect(rename, &QAction::triggered, this, [=]() mutable {
            bool ok;
            QString newName = QInputDialog::getText(this, "Rename", "Enter New Name:",
                                                    QLineEdit::Normal, name, &ok);
            if (ok && !newName.trimmed().isEmpty()) {
                storedData["name"] = newName;
                emit renameItemRequested(storedData);
            }
        });

    } else if (type == "entity") {
        QAction *rename = addAction("Rename");
        QAction *copy = addAction("Copy");
        QAction *deleteEntity = addAction("Delete Entity");

        connect(deleteEntity, &QAction::triggered, this, [=]() {
            emit removeEntityRequested(parentID, ID, false);
        });

        connect(copy, &QAction::triggered, this, [=]() {
            emit copyItemRequested(storedData);
        });

        connect(rename, &QAction::triggered, this, [=]() mutable {
            bool ok;
            QString newName = QInputDialog::getText(this, "Rename", "Enter New Name:",
                                                    QLineEdit::Normal, name, &ok);
            if (ok && !newName.trimmed().isEmpty()) {
                storedData["name"] = newName;
                emit renameItemRequested(storedData);
            }
        });

    } else if (type == "component") {
        QStringList specialComponents = {"radios", "sensors", "iff"};
        if (specialComponents.contains(name.toLower())) {
            QAction *addComponent = addAction("Add");
            QAction *removeComponent = addAction("Remove");

            connect(addComponent, &QAction::triggered, this, [=]() {
                bool ok;
                QString componentName = QInputDialog::getText(this, "Add Component", "Enter Component Name:",
                                                              QLineEdit::Normal, name, &ok);
                if (ok && !componentName.trimmed().isEmpty()) {
                    emit addComponentRequested(parentID, name.toLower(), componentName);
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
}

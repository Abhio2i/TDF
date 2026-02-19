
#include "contextmenu.h"
#include "contextmenu-styles.h"  // Include separate CSS file
#include "GUI/Hierarchytree/additemdialog.h"
#include "core/Hierarchy/hierarchy.h"
#include "qapplication.h"
#include "qprogressdialog.h"
#include "GUI/Hierarchytree/hierarchyconnector.h"
#include "GUI/mainwindow.h"
#include <QInputDialog>
#include <QAction>
#include <QLineEdit>
#include <QTreeWidget>
#include <QMessageBox>

// %%% Constructor %%%
/* Initialize context menu */
ContextMenu::ContextMenu(QWidget *parent)
    : QMenu(parent)
{
    // Apply dark theme to context menu
    setStyleSheet(ContextMenuStyles::ContextMenu);
}

// %%% Menu Setup %%%
/* Setup context menu for specific tree widget item */
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

    // Extract type information from stored data
    if (storedData["type"].type() == QVariant::Map) {
        QVariantMap typeData = storedData["type"].toMap();
        if (typeData.contains("type") && typeData["type"].toString() == "option") {
            type = "profile";
            specificType = typeData.value("value", "").toString();
        } else {
            return;
        }
    } else {
        type = storedData["type"].toString();
    }

    // Setup appropriate menu based on item type
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

// %%% Sub-Component Menu %%%
/* Setup context menu for sub-component items */
void ContextMenu::setupSubComponentMenu(const QVariantMap &data)
{
    QString ID = data["ID"].toString();
    QString parentID = data["parentId"].toString();
    QString name = data["name"].toString();
    QString componentType = data["componentType"].toString();

    QAction *removeSubComponent = addAction("Remove");
    // QAction *rename = addAction("Rename");

    connect(removeSubComponent, &QAction::triggered, this, [=]() {
        emit removeSubComponentRequested(parentID, ID, name);
    });
}

void ContextMenu::setupProfileMenu(const QVariantMap &data)
{
    QString ID = data["ID"].toString();
    QString name = data["name"].toString();
    QString specificType = data["type"].toMap()["value"].toString();

    QString lowerType = specificType.toLower();

    QAction *paste = addAction("Paste");
    QAction *addFolder = addAction("Add Folder");

    // Create appropriate "Add Entity" action based on profile type
    QString addEntityText;

    QMap<QString, QString> specificTypeNames;
    specificTypeNames["specialzone"] = "Special Zone";
    specificTypeNames["radio"] = "Radio";
    specificTypeNames["sensor"] = "Sensor";
    specificTypeNames["weapon"] = "Weapon";
    specificTypeNames["iff"] = "IFF";
    specificTypeNames["formation"] = "Formation";
    specificTypeNames["fixedpoints"] = "Fixed Point";
    specificTypeNames["fixedpoint"] = "Fixed Point";

    if (specificTypeNames.contains(lowerType)) {
        addEntityText = "Add " + specificTypeNames[lowerType];
    } else {
        addEntityText = "Add Entity";
    }

    QAction *addEntity = addAction(addEntityText);
    QAction *deleteProfile = addAction("Delete Profile");

    // Add folder action
    connect(addFolder, &QAction::triggered, this, [=]() {
        bool ok;
        QInputDialog dialog(this);
        dialog.setStyleSheet(ContextMenuStyles::InputDialog);
        dialog.setWindowTitle("Add Folder");
        dialog.setLabelText("Enter Folder Name:");
        dialog.setTextValue("New Folder");

        if (dialog.exec() == QDialog::Accepted) {
            QString folderName = dialog.textValue();
            if (!folderName.trimmed().isEmpty()) {
                if (!folderName.isEmpty()) {
                    folderName[0] = folderName[0].toUpper();
                }
                emit addFolderRequested(ID, folderName, true, QVariantMap());
            }
        }
    });

    connect(addEntity, &QAction::triggered, this, [=]() mutable {
        bool isSensorProfile = (lowerType == "sensor");

        QString editorContext = "";
        MainWindow* mainWindow = MainWindow::instance();

        if (mainWindow) {
            QWidget *currentWidget = mainWindow->stackedWidget->currentWidget();
            if (currentWidget == mainWindow->databaseEditor) {
                editorContext = "database";
            } else if (currentWidget == mainWindow->scenarioEditor) {
                editorContext = "scenario";
            } else if (currentWidget == mainWindow->runtimeEditor) {
                editorContext = "runtime";
            }
        }

        Hierarchy* correctHierarchy = nullptr;
        if (editorContext == "database") {
            if (mainWindow) {
                correctHierarchy = mainWindow->getDatabaseHierarchy();
            }
        } else if (editorContext == "scenario") {
            if (mainWindow && mainWindow->scenarioEditor) {
                correctHierarchy = mainWindow->scenarioEditor->library;
            }
        } else if (editorContext == "runtime") {
            if (mainWindow && mainWindow->runtimeEditor) {
                correctHierarchy = mainWindow->runtimeEditor->library;
            }
        }


        if (!correctHierarchy && mainWindow) {
            correctHierarchy = mainWindow->getDatabaseHierarchy();
        }


        AddItemDialog dialog(AddItemDialog::EntityType,
                             specificType,
                             AddItemDialog::NormalMode,
                             correctHierarchy,
                             this,
                             editorContext);
        dialog.setStyleSheet(ContextMenuStyles::AddItemDialog);

        if (specificTypeNames.contains(lowerType)) {
            dialog.setWindowTitle("Add " + specificTypeNames[lowerType]);
        } else {
            dialog.setWindowTitle("Add Entity");
        }

        if (dialog.exec() == QDialog::Accepted && !dialog.getName().isEmpty()) {
            bool isProfileParent = true;
            QString selectedEntityId;
            if (!selectedEntityId.isEmpty()) {

            }

            if (isSensorProfile) {
                emit addEntityRequested(ID, dialog.getName(),
                                        isProfileParent, dialog.getComponents(), &dialog);
            } else {
                QString progressText;
                if (specificTypeNames.contains(lowerType)) {
                    progressText = "Adding " + specificTypeNames[lowerType] + "s...";
                } else {
                    progressText = "Adding Entities...";
                }
                QProgressDialog progressDialog(progressText,
                                               "Cancel", 0, dialog.getNumber(), this);
                progressDialog.setStyleSheet(ContextMenuStyles::ProgressDialog);
                progressDialog.setWindowFlags(Qt::Dialog | Qt::FramelessWindowHint);
                progressDialog.setWindowModality(Qt::WindowModal);
                progressDialog.setMinimumDuration(0);
                progressDialog.setValue(0);
                progressDialog.show();
                progressDialog.move(this->parentWidget()->mapToGlobal(this->parentWidget()->rect().center()) -
                                    progressDialog.rect().center());

                QApplication::processEvents();

                QString baseName = dialog.getName();
                int count = dialog.getNumber();

                for (int i = 0; i < count; ++i) {
                    if (progressDialog.wasCanceled()) {
                        break;
                    }

                    QString entityName;
                    if (count == 1) {
                        entityName = baseName;
                    } else {
                        entityName = QString("%1-%2").arg(baseName).arg(i + 1);
                    }

                    emit addEntityRequested(ID, entityName,
                                            isProfileParent, dialog.getComponents(), &dialog);

                    progressDialog.setValue(i + 1);
                    QApplication::processEvents();
                }
                progressDialog.close();
            }
        } else {
            // Dialog cancelled
        }
    });

    // Delete profile action
    connect(deleteProfile, &QAction::triggered, this, [=]() {
        QMessageBox msgBox(this);
        msgBox.setStyleSheet(ContextMenuStyles::MessageBox);
        msgBox.setWindowTitle("Delete Profile");
        msgBox.setText(QString("Are you sure you want to delete profile '%1'?").arg(name));
        msgBox.setIcon(QMessageBox::Question);
        msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);

        if (msgBox.exec() == QMessageBox::Yes) {
            emit removeProfileRequested(ID);
        }
    });

    // Paste action
    connect(paste, &QAction::triggered, this, [=]() {
        if (!m_copiedItems.isEmpty()) {
            emit pasteItemsRequested(data, m_copiedItems);
        } else {
            emit pasteItemRequested(data);
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

    // Add folder action
    connect(addFolder, &QAction::triggered, this, [=]() {
        bool ok;
        QInputDialog dialog(this);
        dialog.setStyleSheet(ContextMenuStyles::InputDialog);
        dialog.setWindowTitle("Add Folder");
        dialog.setLabelText("Enter Folder Name:");
        dialog.setTextValue("New Folder");

        if (dialog.exec() == QDialog::Accepted) {
            QString folderName = dialog.textValue();
            if (!folderName.trimmed().isEmpty()) {
                if (!folderName.isEmpty()) {
                    folderName[0] = folderName[0].toUpper();
                }
                emit addFolderRequested(ID, folderName, false, QVariantMap());
            }
        }
    });

    // Add entity action
    connect(addEntity, &QAction::triggered, this, [=]() {
        QString editorContext = "";
        MainWindow* mainWindow = MainWindow::instance();

        if (mainWindow) {
            QWidget *currentWidget = mainWindow->stackedWidget->currentWidget();
            if (currentWidget == mainWindow->databaseEditor) {
                editorContext = "database";
            } else if (currentWidget == mainWindow->scenarioEditor) {
                editorContext = "scenario";
            } else if (currentWidget == mainWindow->runtimeEditor) {
                editorContext = "runtime";
            }
        }

        Hierarchy* correctHierarchy = nullptr;


        if (editorContext == "database") {
            if (mainWindow) {
                correctHierarchy = mainWindow->getDatabaseHierarchy();
            }
        } else if (editorContext == "scenario") {
            if (mainWindow && mainWindow->scenarioEditor) {
                correctHierarchy = mainWindow->scenarioEditor->library;
            }
        } else if (editorContext == "runtime") {
            if (mainWindow && mainWindow->runtimeEditor) {
                correctHierarchy = mainWindow->runtimeEditor->library;
            }
        }

        if (!correctHierarchy) {
            return;
        }

        AddItemDialog dialog(AddItemDialog::EntityType, "",
                             AddItemDialog::NormalMode,
                             correctHierarchy,
                             this,
                             editorContext);
        dialog.setStyleSheet(ContextMenuStyles::AddItemDialog);

        if (dialog.exec() == QDialog::Accepted && !dialog.getName().isEmpty()) {
            QString baseName = dialog.getName();
            int count = dialog.getNumber();

            for (int i = 0; i < count; ++i) {
                QString entityName;
                if (count == 1) {
                    entityName = baseName;
                } else {
                    entityName = QString("%1-%2").arg(baseName).arg(i + 1);
                }

                emit addEntityRequested(ID, entityName,
                                        false, dialog.getComponents(), &dialog);
            }
        }
    });

    connect(deleteFolder, &QAction::triggered, this, [=]() {
        QMessageBox msgBox(this);
        msgBox.setStyleSheet(ContextMenuStyles::MessageBox);
        msgBox.setWindowTitle("Delete Folder");
        msgBox.setText(QString("Are you sure you want to delete folder '%1'?").arg(name));
        msgBox.setIcon(QMessageBox::Question);
        msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);

        if (msgBox.exec() == QMessageBox::Yes) {
            emit removeFolderRequested(parentID, ID, false);
        }
    });

    connect(paste, &QAction::triggered, this, [=]() {
        if (!m_copiedItems.isEmpty()) {
            emit pasteItemsRequested(data, m_copiedItems);
        } else {
            emit pasteItemRequested(data);
        }
    });

    // Rename action
    connect(rename, &QAction::triggered, this, [=]() mutable {
        QInputDialog dialog(this);
        dialog.setStyleSheet(ContextMenuStyles::InputDialog);
        dialog.setWindowTitle("Rename");
        dialog.setLabelText("Enter New Name:");
        dialog.setTextValue(name);

        if (dialog.exec() == QDialog::Accepted) {
            QString newName = dialog.textValue();
            if (!newName.trimmed().isEmpty()) {
                QVariantMap modifiedData = data;
                modifiedData["name"] = newName;
                emit renameItemRequested(modifiedData);
            }
        }
    });
}

// %%% Entity Menu %%%
/* Setup context menu for entity items */
void ContextMenu::setupEntityMenu(const QVariantMap &data)
{
    QString ID = data["ID"].toString();
    QString parentID = data["parentId"].toString();
    QString name = data["name"].toString();

    QAction *rename = addAction("Rename");
    QAction *copy = addAction("Copy");
    QAction *deleteEntity = addAction("Delete Entity");

    // Delete entity action
    connect(deleteEntity, &QAction::triggered, this, [=]() {
        QMessageBox msgBox(this);
        msgBox.setStyleSheet(ContextMenuStyles::MessageBox);
        msgBox.setWindowTitle("Delete Entity");
        msgBox.setText(QString("Are you sure you want to delete entity '%1'?").arg(name));
        msgBox.setIcon(QMessageBox::Question);
        msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);

        if (msgBox.exec() == QMessageBox::Yes) {
            emit removeEntityRequested(parentID, ID, false);
        }
    });

    // Copy action
    connect(copy, &QAction::triggered, this, [=]() {
        emit copyItemRequested(data);
    });

    // Rename action
    connect(rename, &QAction::triggered, this, [=]() mutable {
        QInputDialog dialog(this);
        dialog.setStyleSheet(ContextMenuStyles::InputDialog);
        dialog.setWindowTitle("Rename");
        dialog.setLabelText("Enter New Name:");
        dialog.setTextValue(name);

        if (dialog.exec() == QDialog::Accepted) {
            QString newName = dialog.textValue();
            if (!newName.trimmed().isEmpty()) {
                QVariantMap modifiedData = data;
                modifiedData["name"] = newName;
                emit renameItemRequested(modifiedData);
            }
        }
    });
}

// %%% Component Menu %%%
/* Setup context menu for component items */
void ContextMenu::setupComponentMenu(const QVariantMap &data)
{
    QString ID = data["ID"].toString();
    QString parentID = data["parentId"].toString();
    QString name = data["name"].toString();
    QStringList specialComponents = {"radios", "sensors", "iffs"};
    if (specialComponents.contains(name.toLower())) {
        QAction *addComponent = addAction("Add");
        // QAction *removeComponent = addAction("Remove");
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
            Hierarchy* dbHierarchy = MainWindow::instance()->getDatabaseHierarchy();
            AddItemDialog dialog(AddItemDialog::EntityType,
                                 componentType,
                                 mode,
                                 dbHierarchy,
                                 this);
            dialog.setStyleSheet(ContextMenuStyles::AddItemDialog);
            if (dialog.exec() == QDialog::Accepted && !dialog.getName().isEmpty()) {
                QString profileName = dialog.getProfileName();
                QString profileId = dialog.getProfileId();
                QString sensorType = (componentType == "sensors") ?
                                         dialog.getSensorType() : "";
                emit addComponentRequested(ID, componentType, dialog.getName(),
                                           sensorType, profileId);
            }
        });
    } else {

    }
}

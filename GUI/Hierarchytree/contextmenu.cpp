/* =============================================================================
 * FILE:         additemdialog.cpp
 * MODULE:       Add Item Dialog
 * PROJECT:      Indigenous Scenario and Sensor Simulation Toolkit (ISSST)
 * ORGANISATION: Oxygen 2 Innovation (O2I).
 * STANDARD:     RTCA DO-178C / ED-12C, DAL B
 * COVERAGE:     Branch / Decision Coverage required (100% true/false paths)
 *
 * DESCRIPTION:  Implements the AddItemDialog class which provides a modal
 *               dialog for adding new items (entities or folders) with
 *               configurable properties and components. Supports entity/folder
 *               creation, sensor/IFF/radio/weapon component configuration,
 *               scenario parameters (range, speed, turn radius, trajectory),
 *               profile selection, and entity component inheritance. The dialog
 *               adapts its UI based on dialog type (Entity/Folder) and mode
 *               (Normal, ComponentSensor, ComponentIFF, ComponentRadio,
 *               ComponentWeapon). Includes searchable entity selection with
 *               autocompletion, city data loading from JSON, and comprehensive
 *               input validation.
 *
 * REQUIREMENTS: Implements REQ-DIALOG-010 through REQ-DIALOG-018
 *
 * AUTHOR:       Arti Rajpoot
 * REVIEWED BY:  [Reviewer Name], [Review Date] — SPR-DIALOG-001
 *
 *
 * COPYRIGHT:    Oxygen 2 Innovation (O2I). All rights reserved.
 *               Restricted circulation — defence simulation use only.
 * =============================================================================
 */
#include "contextmenu.h"
#include "contextmenu-styles.h"
#include "GUI/Hierarchytree/additemdialog.h"
#include "GUI/Hierarchytree/addweapondialog.h"
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
// ── Weapon subclass includes ──────────────────────────────────────────────────
#include "core/Hierarchy/EntityProfiles/weapons/missile.h"
#include "core/Hierarchy/EntityProfiles/weapons/bomb.h"
#include "core/Hierarchy/EntityProfiles/weapons/torpedo.h"
#include "core/Hierarchy/EntityProfiles/weapons/artillery.h"
#include "core/Hierarchy/EntityProfiles/weapons/rocket.h"
#include "core/Hierarchy/EntityProfiles/weapons/flare.h"
#include "core/Hierarchy/EntityProfiles/weapons/chaff.h"

static Weapon* createWeapon(const QString& typeName, Hierarchy* h)
{
    if (typeName == "Bomb")      return new Bomb(h);
    if (typeName == "Torpedo")   return new Torpedo(h);
    if (typeName == "Artillery") return new Artillery(h);
    if (typeName == "Rocket")    return new Rocket(h);
    if (typeName == "Flare")     return new Flare(h);
    if (typeName == "Chaff")     return new Chaff(h);
    return new Missile(h);   // default
}

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
    QString type;

    if (storedData["type"].type() == QVariant::Map) {
        QVariantMap typeData = storedData["type"].toMap();
        if (typeData.contains("type") && typeData["type"].toString() == "option") {
            type = "profile";
        } else {
            return;
        }
    } else {
        type = storedData["type"].toString();
    }
    if (type == "component") {
        return;
    }
    if      (type == "subcomponent") setupSubComponentMenu(storedData);
    else if (type == "profile")      setupProfileMenu(storedData);
    else if (type == "folder")       setupFolderMenu(storedData);
    else if (type == "entity")       setupEntityMenu(storedData);
    else if (type == "component")    setupComponentMenu(storedData);
}
// %%% Sub-Component Menu %%%
/* Setup context menu for sub-component items */
void ContextMenu::setupSubComponentMenu(const QVariantMap &data)
{
    QString ID           = data["ID"].toString();
    QString parentID     = data["parentId"].toString();
    QString name         = data["name"].toString();

    QAction *removeSubComponent = addAction(QIcon(":/icons/images/delete.png"),  "Remove");
    QAction *rename             = addAction(QIcon(":/icons/images/rename.png"),    "Rename");

    connect(removeSubComponent, &QAction::triggered, this, [=]() {
        emit removeSubComponentRequested(parentID, ID, name);
    });

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

// %%% Profile Menu %%%
void ContextMenu::setupProfileMenu(const QVariantMap &data)
{
    QString ID           = data["ID"].toString();
    QString name         = data["name"].toString();
    QString specificType = data["type"].toMap()["value"].toString();
    QString lowerType    = specificType.toLower();

    QAction *paste      = addAction(QIcon(":/icons/images/paste.png"),      "Paste");
    QAction *addFolder  = addAction(QIcon(":/icons/images/add.png"), "Add Folder");

    QMap<QString, QString> specificTypeNames;
    specificTypeNames["specialzone"]  = "Special Zone";
    specificTypeNames["radio"]        = "Radio";
    specificTypeNames["sensor"]       = "Sensor";
    specificTypeNames["weapon"]       = "Weapon";
    specificTypeNames["iff"]          = "IFF";
    specificTypeNames["formation"]    = "Formation";
    specificTypeNames["fixedpoints"]  = "Fixed Point";
    specificTypeNames["fixedpoint"]   = "Fixed Point";

    QString addEntityText = specificTypeNames.contains(lowerType)
                                ? "Add " + specificTypeNames[lowerType]
                                : "Add Entity";

    QAction *addEntity     = addAction(QIcon(":/icons/images/add.png"), addEntityText);
    // ── Add Folder ────────────────────────────────────────────────────────
    connect(addFolder, &QAction::triggered, this, [=]() {
        QInputDialog dialog(this);
        dialog.setStyleSheet(ContextMenuStyles::InputDialog);
        dialog.setWindowTitle("Add Folder");
        dialog.setLabelText("Enter Folder Name:");
        dialog.setTextValue("New Folder");

        if (dialog.exec() == QDialog::Accepted) {
            QString folderName = dialog.textValue();
            if (!folderName.trimmed().isEmpty()) {
                if (!folderName.isEmpty()) folderName[0] = folderName[0].toUpper();
                emit addFolderRequested(ID, folderName, true, QVariantMap());
            }
        }
    });

    // ── Add Entity ────────────────────────────────────────────────────────
    connect(addEntity, &QAction::triggered, this, [=]() mutable {
        bool isSensorProfile = (lowerType == "sensor");
        bool isWeaponProfile = (lowerType == "weapon");

        QString editorContext = "";
        MainWindow* mainWindow = MainWindow::instance();
        if (mainWindow) {
            QWidget *currentWidget = mainWindow->stackedWidget->currentWidget();
            if      (currentWidget == mainWindow->databaseEditor) editorContext = "database";
            else if (currentWidget == mainWindow->scenarioEditor) editorContext = "scenario";
            else if (currentWidget == mainWindow->runtimeEditor)  editorContext = "runtime";
        }

        Hierarchy* correctHierarchy = nullptr;
        if (isWeaponProfile) {
            if      (editorContext == "database" && mainWindow) correctHierarchy = mainWindow->getDatabaseHierarchy();
            else if (editorContext == "scenario" && mainWindow && mainWindow->scenarioEditor) correctHierarchy = mainWindow->scenarioEditor->hierarchy;
            else if (editorContext == "runtime"  && mainWindow && mainWindow->runtimeEditor)  correctHierarchy = mainWindow->runtimeEditor->hierarchy;
            if (!correctHierarchy && mainWindow) correctHierarchy = mainWindow->getDatabaseHierarchy();
        } else {
            if      (editorContext == "database" && mainWindow) correctHierarchy = mainWindow->getDatabaseHierarchy();
            else if (editorContext == "scenario" && mainWindow && mainWindow->scenarioEditor) correctHierarchy = mainWindow->scenarioEditor->library;
            else if (editorContext == "runtime"  && mainWindow && mainWindow->runtimeEditor)  correctHierarchy = mainWindow->runtimeEditor->library;
            if (!correctHierarchy && mainWindow) correctHierarchy = mainWindow->getDatabaseHierarchy();
        }

        // ── Weapon Profile ────────────────────────────────────────────────
        if (isWeaponProfile) {
            if (!correctHierarchy) { QMessageBox::critical(this, "Error", "Hierarchy not available"); return; }
            Hierarchy* dbHierarchy = mainWindow ? mainWindow->getDatabaseHierarchy() : nullptr;

            if (editorContext == "database") {
                AddWeaponDialog dlg(parentWidget(), dbHierarchy, true);
                dlg.setStyleSheet(ContextMenuStyles::AddItemDialog);
                if (dlg.exec() != QDialog::Accepted) return;
                QString weaponName = dlg.weaponName().trimmed();
                if (weaponName.isEmpty()) { QMessageBox::warning(this, "Validation Error", "Weapon name cannot be empty"); return; }
                try {
                    QString typeName = dlg.weaponTypeStr();
                    Weapon* weapon   = createWeapon(typeName, correctHierarchy);
                    weapon->Name     = weaponName.toStdString();
                    weapon->parentID = ID.toStdString();
                    auto profIt = correctHierarchy->ProfileCategories.find(ID.toStdString());
                    if (profIt == correctHierarchy->ProfileCategories.end() || !profIt->second) {
                        QMessageBox::critical(this, "Error", "Weapon profile not found in hierarchy");
                        delete weapon; return;
                    }
                    profIt->second->addEntityWithObject(weapon);
                    QJsonObject config = dlg.configJson();
                    config["weaponTypeName"] = typeName;
                    weapon->fromJson(config);
                    weapon->Name     = weaponName.toStdString();
                    weapon->parentID = ID.toStdString();
                    weapon->syncComponentsFromWeaponData();
                } catch (const std::exception& e) {
                    QMessageBox::critical(this, "Error", QString("Failed to create weapon: %1").arg(e.what()));
                }
                return;
            }

            AddWeaponDialog dlg(parentWidget(), dbHierarchy, false);
            dlg.setStyleSheet(ContextMenuStyles::AddItemDialog);
            if (dlg.exec() != QDialog::Accepted) return;
            QString selectedId = dlg.selectedEntityId();
            QString weaponName = dlg.weaponName().trimmed();
            if (selectedId.isEmpty()) { QMessageBox::warning(this, "Validation Error", "Please select a weapon from the database"); return; }
            if (weaponName.isEmpty()) { QMessageBox::warning(this, "Validation Error", "Weapon name cannot be empty"); return; }
            try {
                QString typeName = "Missile";
                if (dbHierarchy) {
                    auto it = dbHierarchy->Weapons.find(selectedId.toStdString());
                    if (it != dbHierarchy->Weapons.end() && it->second)
                        typeName = it->second->weaponTypeName();
                }
                Weapon* weapon = createWeapon(typeName, correctHierarchy);
                weapon->Name     = weaponName.toStdString();
                weapon->parentID = ID.toStdString();
                if (dbHierarchy) {
                    auto it = dbHierarchy->Weapons.find(selectedId.toStdString());
                    if (it != dbHierarchy->Weapons.end() && it->second) {
                        std::string savedId = weapon->ID, savedName = weapon->Name, savedParent = weapon->parentID;
                        weapon->fromJson(it->second->toJson());
                        weapon->ID = savedId; weapon->Name = savedName; weapon->parentID = savedParent;
                    }
                }
                auto profIt = correctHierarchy->ProfileCategories.find(ID.toStdString());
                if (profIt == correctHierarchy->ProfileCategories.end() || !profIt->second) {
                    QMessageBox::critical(this, "Error", "Weapon profile not found in hierarchy");
                    delete weapon; return;
                }
                profIt->second->addEntityWithObject(weapon);
                weapon->syncComponentsFromWeaponData();
            } catch (const std::exception& e) {
                QMessageBox::critical(this, "Error", QString("Failed to create weapon: %1").arg(e.what()));
            }
            return;
        }

        // ── All Other Profiles ────────────────────────────────────────────
        AddItemDialog dialog(AddItemDialog::EntityType, specificType,
                             AddItemDialog::NormalMode, correctHierarchy, this, editorContext);
        dialog.setStyleSheet(ContextMenuStyles::AddItemDialog);
        if (specificTypeNames.contains(lowerType))
            dialog.setWindowTitle("Add " + specificTypeNames[lowerType]);
        else
            dialog.setWindowTitle("Add Entity");

        if (dialog.exec() == QDialog::Accepted && !dialog.getName().isEmpty()) {
            bool isProfileParent = true;
            if (isSensorProfile) {
                emit addEntityRequested(ID, dialog.getName(), isProfileParent, dialog.getComponents(), &dialog);
            } else {
                QString progressText = specificTypeNames.contains(lowerType)
                                           ? "Adding " + specificTypeNames[lowerType] + "s..."
                                           : "Adding Entities...";
                QProgressDialog progressDialog(progressText, "Cancel", 0, dialog.getNumber(), this);
                progressDialog.setStyleSheet(ContextMenuStyles::ProgressDialog);
                progressDialog.setWindowFlags(Qt::Dialog | Qt::FramelessWindowHint);
                progressDialog.setWindowModality(Qt::WindowModal);
                progressDialog.setMinimumDuration(0);
                progressDialog.setValue(0);
                progressDialog.show();
                progressDialog.move(this->parentWidget()->mapToGlobal(
                                        this->parentWidget()->rect().center()) - progressDialog.rect().center());
                QApplication::processEvents();

                QString baseName = dialog.getName();
                int count = dialog.getNumber();
                for (int i = 0; i < count; ++i) {
                    if (progressDialog.wasCanceled()) break;
                    QString entityName = (count == 1) ? baseName : QString("%1-%2").arg(baseName).arg(i + 1);
                    emit addEntityRequested(ID, entityName, isProfileParent, dialog.getComponents(), &dialog);
                    progressDialog.setValue(i + 1);
                    QApplication::processEvents();
                }
                progressDialog.close();
            }
        }
    });

    // // ── Delete Profile ────────────────────────────────────────────────────
    // connect(deleteProfile, &QAction::triggered, this, [=]() {
    //     QMessageBox msgBox(this);
    //     msgBox.setStyleSheet(ContextMenuStyles::MessageBox);
    //     msgBox.setWindowTitle("Delete Profile");
    //     msgBox.setText(QString("Are you sure you want to delete profile '%1'?").arg(name));
    //     msgBox.setIcon(QMessageBox::Question);
    //     msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
    //     if (msgBox.exec() == QMessageBox::Yes) emit removeProfileRequested(ID);
    // });

    // ── Paste ─────────────────────────────────────────────────────────────
    connect(paste, &QAction::triggered, this, [=]() {
        if (!m_copiedItems.isEmpty()) emit pasteItemsRequested(data, m_copiedItems);
        else                          emit pasteItemRequested(data);
    });
}

void ContextMenu::setupFolderMenu(const QVariantMap &data)
{
    QString ID       = data["ID"].toString();
    QString parentID = data["parentId"].toString();
    QString name     = data["name"].toString();

    // ── Parent profile type nikalo ─────────────────────────────────────
    QString parentProfileType;
    QTreeWidget* treeWidget = nullptr;
    if (parentWidget()) {
        treeWidget = parentWidget()->findChild<QTreeWidget*>();
    }
    if (treeWidget) {
        QTreeWidgetItemIterator it(treeWidget);
        while (*it) {
            QVariantMap pData = (*it)->data(0, Qt::UserRole).toMap();
            if (pData["ID"].toString() == parentID) {
                // Direct profile parent
                if (pData["type"].type() == QVariant::Map) {
                    QVariantMap typeData = pData["type"].toMap();
                    if (typeData["type"].toString() == "option") {
                        parentProfileType = typeData["value"].toString();
                    }
                }
                // Folder parent — grandparent check
                else if (pData["type"].toString() == "folder") {
                    QString gpID = pData["parentId"].toString();
                    QTreeWidgetItemIterator git(treeWidget);
                    while (*git) {
                        QVariantMap gpData = (*git)->data(0, Qt::UserRole).toMap();
                        if (gpData["ID"].toString() == gpID) {
                            if (gpData["type"].type() == QVariant::Map) {
                                QVariantMap gpTypeData = gpData["type"].toMap();
                                if (gpTypeData["type"].toString() == "option") {
                                    parentProfileType = gpTypeData["value"].toString();
                                }
                            }
                            break;
                        }
                        ++git;
                    }
                }
                break;
            }
            ++it;
        }
    }

    // ── Add Entity label profile ke hisaab se ─────────────────────────
    QMap<QString, QString> profileLabelMap;
    profileLabelMap["Sensor"]      = "Add Sensor";
    profileLabelMap["IFF"]         = "Add IFF";
    profileLabelMap["Radio"]       = "Add Radio";
    profileLabelMap["Weapon"]      = "Add Weapon";
    profileLabelMap["Formation"]   = "Add Formation";
    profileLabelMap["FixedPoints"] = "Add Fixed Point";
    profileLabelMap["SpecialZone"] = "Add Special Zone";
    QString addEntityLabel = profileLabelMap.contains(parentProfileType)
                                 ? profileLabelMap[parentProfileType]
                                 : "Add Entity";

    // ── Menu actions ──────────────────────────────────────────────────
    QAction *rename       = addAction(QIcon(":/icons/images/rename.png"),   "Rename");
    QAction *paste        = addAction(QIcon(":/icons/images/paste.png"),    "Paste");
    QAction *addFolder    = addAction(QIcon(":/icons/images/add.png"),      "Add Folder");
    QAction *addEntity    = addAction(QIcon(":/icons/images/add.png"),      addEntityLabel);
    QAction *deleteFolder = addAction(QIcon(":/icons/images/delete.png"),   "Delete Folder");

    // ── Add Folder ────────────────────────────────────────────────────
    connect(addFolder, &QAction::triggered, this, [=]() {
        QInputDialog dialog(this);
        dialog.setStyleSheet(ContextMenuStyles::InputDialog);
        dialog.setWindowTitle("Add Folder");
        dialog.setLabelText("Enter Folder Name:");
        dialog.setTextValue("New Folder");
        if (dialog.exec() == QDialog::Accepted) {
            QString folderName = dialog.textValue();
            if (!folderName.trimmed().isEmpty()) {
                if (!folderName.isEmpty()) folderName[0] = folderName[0].toUpper();
                emit addFolderRequested(ID, folderName, false, QVariantMap());
            }
        }
    });

    // ── Add Entity (profile-aware) ────────────────────────────────────
    connect(addEntity, &QAction::triggered, this, [=]() {
        QString editorContext = "";
        MainWindow* mainWindow = MainWindow::instance();
        if (mainWindow) {
            QWidget *currentWidget = mainWindow->stackedWidget->currentWidget();
            if      (currentWidget == mainWindow->databaseEditor) editorContext = "database";
            else if (currentWidget == mainWindow->scenarioEditor) editorContext = "scenario";
            else if (currentWidget == mainWindow->runtimeEditor)  editorContext = "runtime";
        }

        bool isWeaponProfile = (parentProfileType == "Weapon");

        Hierarchy* correctHierarchy = nullptr;
        if (isWeaponProfile) {
            if      (editorContext == "database" && mainWindow) correctHierarchy = mainWindow->getDatabaseHierarchy();
            else if (editorContext == "scenario" && mainWindow && mainWindow->scenarioEditor) correctHierarchy = mainWindow->scenarioEditor->hierarchy;
            else if (editorContext == "runtime"  && mainWindow && mainWindow->runtimeEditor)  correctHierarchy = mainWindow->runtimeEditor->hierarchy;
            if (!correctHierarchy && mainWindow) correctHierarchy = mainWindow->getDatabaseHierarchy();
        } else {
            if      (editorContext == "database" && mainWindow) correctHierarchy = mainWindow->getDatabaseHierarchy();
            else if (editorContext == "scenario" && mainWindow && mainWindow->scenarioEditor) correctHierarchy = mainWindow->scenarioEditor->library;
            else if (editorContext == "runtime"  && mainWindow && mainWindow->runtimeEditor)  correctHierarchy = mainWindow->runtimeEditor->library;
            if (!correctHierarchy && mainWindow) correctHierarchy = mainWindow->getDatabaseHierarchy();
        }
        if (!correctHierarchy) return;

        // Weapon profile folder
        if (isWeaponProfile) {
            Hierarchy* dbHierarchy = mainWindow ? mainWindow->getDatabaseHierarchy() : nullptr;
            AddWeaponDialog dlg(parentWidget(), dbHierarchy, (editorContext == "database"));
            dlg.setStyleSheet(ContextMenuStyles::AddItemDialog);
            if (dlg.exec() != QDialog::Accepted) return;
            QString weaponName = dlg.weaponName().trimmed();
            if (weaponName.isEmpty()) return;
            QString typeName = dlg.weaponTypeStr();
            Weapon* weapon = createWeapon(typeName, correctHierarchy);
            weapon->Name     = weaponName.toStdString();
            weapon->parentID = ID.toStdString();

            if (editorContext != "database" && dbHierarchy) {
                QString selId = dlg.selectedEntityId();
                if (!selId.isEmpty()) {
                    auto it = dbHierarchy->Weapons.find(selId.toStdString());
                    if (it != dbHierarchy->Weapons.end() && it->second) {
                        std::string savedId = weapon->ID, savedName = weapon->Name, savedParent = weapon->parentID;
                        weapon->fromJson(it->second->toJson());
                        weapon->ID = savedId; weapon->Name = savedName; weapon->parentID = savedParent;
                    }
                }
            } else {
                QJsonObject config = dlg.configJson();
                config["weaponTypeName"] = typeName;
                weapon->fromJson(config);
                weapon->Name     = weaponName.toStdString();
                weapon->parentID = ID.toStdString();
            }

            // Folder ke andar add karo
            auto folderIt = correctHierarchy->Folders.find(ID.toStdString());
            if (folderIt != correctHierarchy->Folders.end() && folderIt->second) {
                folderIt->second->addEntityWithObject(weapon);
            }
            weapon->syncComponentsFromWeaponData();
            return;
        }

        QString specificTypeForDialog = parentProfileType.isEmpty() ? "Platform" : parentProfileType;
        AddItemDialog dialog(AddItemDialog::EntityType,
                             specificTypeForDialog,
                             AddItemDialog::NormalMode,
                             correctHierarchy, this, editorContext);
        dialog.setStyleSheet(ContextMenuStyles::AddItemDialog);
        dialog.setWindowTitle(addEntityLabel);

        if (dialog.exec() == QDialog::Accepted && !dialog.getName().isEmpty()) {
            QString baseName = dialog.getName();
            int count = dialog.getNumber();
            for (int i = 0; i < count; ++i) {
                QString entityName = (count == 1) ? baseName : QString("%1-%2").arg(baseName).arg(i + 1);
                emit addEntityRequested(ID, entityName, false, dialog.getComponents(), &dialog);
            }
        }
    });

    // ── Delete Folder ─────────────────────────────────────────────────
    connect(deleteFolder, &QAction::triggered, this, [=]() {
        QMessageBox msgBox(this);
        msgBox.setStyleSheet(ContextMenuStyles::MessageBox);
        msgBox.setWindowTitle("Delete Folder");
        msgBox.setText(QString("Are you sure you want to delete folder '%1'?").arg(name));
        msgBox.setIcon(QMessageBox::Question);
        msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
        if (msgBox.exec() == QMessageBox::Yes) emit removeFolderRequested(parentID, ID, false);
    });

    // ── Paste ─────────────────────────────────────────────────────────
    connect(paste, &QAction::triggered, this, [=]() {
        if (!m_copiedItems.isEmpty()) emit pasteItemsRequested(data, m_copiedItems);
        else                          emit pasteItemRequested(data);
    });

    // ── Rename ────────────────────────────────────────────────────────
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

void ContextMenu::setupEntityMenu(const QVariantMap &data)
{
    QString ID       = data["ID"].toString();
    QString parentID = data["parentId"].toString();
    QString name     = data["name"].toString();

    bool isPlatformEntity = false;
    QTreeWidget* treeWidget = nullptr;
    if (parentWidget()) {
        treeWidget = parentWidget()->findChild<QTreeWidget*>();
    }
    if (treeWidget) {
        QTreeWidgetItemIterator it(treeWidget);
        while (*it) {
            QVariantMap pData = (*it)->data(0, Qt::UserRole).toMap();
            if (pData["ID"].toString() == parentID) {
                QString pType = pData["type"].toString();

                // Case 1: Direct profile parent
                if (pData["type"].type() == QVariant::Map) {
                    QVariantMap typeData = pData["type"].toMap();
                    if (typeData["value"].toString() == "Platform") {
                        isPlatformEntity = true;
                    }
                }
                else if (pType == "folder") {
                    QString grandParentID = pData["parentId"].toString();
                    QTreeWidgetItemIterator git(treeWidget);
                    while (*git) {
                        QVariantMap gpData = (*git)->data(0, Qt::UserRole).toMap();
                        if (gpData["ID"].toString() == grandParentID) {
                            if (gpData["type"].type() == QVariant::Map) {
                                QVariantMap gpTypeData = gpData["type"].toMap();
                                if (gpTypeData["value"].toString() == "Platform") {
                                    isPlatformEntity = true;
                                }
                            }
                            break;
                        }
                        ++git;
                    }
                }
                break;
            }
            ++it;
        }
    }
    QAction *rename      = addAction(QIcon(":/icons/images/rename.png"), "Rename");
    QAction *copy        = addAction(QIcon(":/icons/images/copy.png"),   "Copy");
    QAction *setActive   = nullptr;
    QAction *setInactive = nullptr;
    QMenu *addComponentMenu = nullptr;
    QAction *addWeapon = nullptr, *addSensor = nullptr, *addIFF = nullptr, *addRadio = nullptr;
    QMenu *setTeamMenu = nullptr;
    QMenu *setCategoryMenu = nullptr;

    if (isPlatformEntity) {
        setActive   = addAction(QIcon(":/icons/images/enable.png"),  "Set Active");
        setInactive = addAction(QIcon(":/icons/images/disable.png"), "Set Inactive");

        addComponentMenu = addMenu(QIcon(":/icons/images/add.png"), "Add");
        addComponentMenu->setStyleSheet(styleSheet());
        addWeapon = addComponentMenu->addAction(QIcon(":/icons/images/bio-weapon.png"),      "Add Weapon");
        addSensor = addComponentMenu->addAction(QIcon(":/icons/images/database (1).png"),    "Add Sensor");
        addIFF    = addComponentMenu->addAction(QIcon(":/icons/images/identification.png"),  "Add IFF");
        addRadio  = addComponentMenu->addAction(QIcon(":/icons/images/radio.png"),           "Add Radio");

        setTeamMenu = addMenu(QIcon(":/icons/images/seteam.png"), "Set Team");
        setTeamMenu->setStyleSheet(styleSheet());
        const QStringList teams = {"RedTeam", "BlueTeam", "GreenTeam", "YellowTeam",
                                   "GreyTeam", "AlphaTeam", "BetaTeam", "GammaTeam"};
        for (const QString& team : teams) {
            QAction *teamAction = setTeamMenu->addAction(team);
            connect(teamAction, &QAction::triggered, this, [=]() {
                emit addTeamToEntityRequested(data, team);
            });
        }

        QString editorCtx = "";
        MainWindow* mw = MainWindow::instance();
        if (mw) {
            QWidget *cur = mw->stackedWidget->currentWidget();
            if      (cur == mw->databaseEditor) editorCtx = "database";
            else if (cur == mw->scenarioEditor) editorCtx = "scenario";
            else if (cur == mw->runtimeEditor)  editorCtx = "runtime";
        }

        if (editorCtx == "database") {
            setCategoryMenu = addMenu(QIcon(":/icons/images/set.png"), "Set Category");
            setCategoryMenu->setStyleSheet(styleSheet());
            const QStringList categories = {"Aircraft", "Helicopter", "Ship", "Submarine", "Tank"};
            for (const QString& category : categories) {
                QAction *categoryAction = setCategoryMenu->addAction(category);
                connect(categoryAction, &QAction::triggered, this, [=]() {
                    emit setCategoryToEntityRequested(data, category);
                });
            }
        }
    }
    QAction *deleteEntity = addAction(QIcon(":/icons/images/delete.png"), "Delete Entity");

    // ── Connects for standard actions ──────────────────────────────────────
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
    connect(copy, &QAction::triggered, this, [=]() { emit copyItemRequested(data); });
    connect(deleteEntity, &QAction::triggered, this, [=]() {
        QMessageBox msgBox(this);
        msgBox.setStyleSheet(ContextMenuStyles::MessageBox);
        msgBox.setWindowTitle("Delete Entity");
        msgBox.setText(QString("Are you sure you want to delete entity '%1'?").arg(name));
        msgBox.setIcon(QMessageBox::Question);
        msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
        if (msgBox.exec() == QMessageBox::Yes) emit removeEntityRequested(parentID, ID, false);
    });

    if (isPlatformEntity) {
        connect(setActive,   &QAction::triggered, this, [=]() { emit setEntityActiveRequested(ID, true); });
        connect(setInactive, &QAction::triggered, this, [=]() { emit setEntityActiveRequested(ID, false); });
        connect(addWeapon,   &QAction::triggered, this, [=]() { emit addWeaponToEntityRequested(data); });
        connect(addSensor,   &QAction::triggered, this, [=]() { emit addSensorToEntityRequested(data); });
        connect(addIFF,      &QAction::triggered, this, [=]() { emit addIFFToEntityRequested(data); });
        connect(addRadio,    &QAction::triggered, this, [=]() { emit addRadioToEntityRequested(data); });
    }

    //  Formation "Select with Entity" action ─────────────────────────
    if (m_hierarchy) {
        auto it = m_hierarchy->Entities.find(ID.toStdString());
        if (it != m_hierarchy->Entities.end()) {
            Formation* formation = dynamic_cast<Formation*>(it->second);
            if (formation) {
                addSeparator();
                QAction* selectWithEntityAction = addAction("Select with Entity");
                connect(selectWithEntityAction, &QAction::triggered, this, [this, ID]() {
                    emit selectFormationWithEntityRequested(ID);
                });
            }
        }
    }
}
// %%% Component Menu %%%
/* Setup context menu for component items */
void ContextMenu::setupComponentMenu(const QVariantMap &data)
{
    QString ID = data["ID"].toString();
    QString parentID = data["parentId"].toString();
    QString name = data["name"].toString();
    if (name.toLower() == "weapons") {
        QAction *addWeaponAction = addAction("Add");
        connect(addWeaponAction, &QAction::triggered, this, [=]() {
            MainWindow* mainWindow = MainWindow::instance();
            Hierarchy* correctHierarchy = nullptr;
            if (mainWindow) {
                QWidget *cur = mainWindow->stackedWidget->currentWidget();
                if (cur == mainWindow->databaseEditor)
                    correctHierarchy = mainWindow->getDatabaseHierarchy();
                else if (cur == mainWindow->scenarioEditor && mainWindow->scenarioEditor)
                    correctHierarchy = mainWindow->scenarioEditor->hierarchy;
                else if (cur == mainWindow->runtimeEditor && mainWindow->runtimeEditor)
                    correctHierarchy = mainWindow->runtimeEditor->hierarchy;
            }
            if (!correctHierarchy) return;
            AddWeaponDialog dlg(parentWidget(), nullptr, true);
            dlg.setStyleSheet(ContextMenuStyles::AddItemDialog);
            if (dlg.exec() != QDialog::Accepted) return;
            QString weaponName = dlg.weaponName().trimmed();
            if (weaponName.isEmpty()) return;
            emit addComponentRequested(parentID, "weapons", weaponName,
                                       dlg.weaponTypeStr(), "");
            if (correctHierarchy->Entities.count(parentID.toStdString())) {
                Entity* ent = (correctHierarchy->Entities)[parentID.toStdString()];
                Platform* plf = dynamic_cast<Platform*>(ent);
                if (plf && plf->weapons) {
                    for (auto& [wid, w] : *plf->weapons->weapons) {
                        if (QString::fromStdString(w->Name) == weaponName) {
                            QJsonObject cfg = dlg.configJson();
                            cfg["weaponTypeName"] = dlg.weaponTypeStr();
                            w->fromJson(cfg);
                            w->Name = weaponName.toStdString();
                            w->syncComponentsFromWeaponData();
                            plf->weapons->updateSubComponent(wid, w->toJson());
                            break;
                        }
                    }
                }
            }
        });
        return;
    }

    QStringList specialComponents = {"radios", "sensors", "iffs"};
    if (specialComponents.contains(name.toLower())) {
        QAction *addComponent = addAction("Add");
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
                QString profileId   = dialog.getProfileId();
                QString sensorType  = (componentType == "sensors") ?
                                         dialog.getSensorType() : "";
                emit addComponentRequested(ID, componentType, dialog.getName(),
                                           sensorType, profileId);
            }
        });

    }


}

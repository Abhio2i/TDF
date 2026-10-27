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
#include "core/Hierarchy/EntityProfiles/weapons/missile.h"
#include "core/Hierarchy/EntityProfiles/weapons/bomb.h"
#include "core/Hierarchy/EntityProfiles/weapons/torpedo.h"
#include "core/Hierarchy/EntityProfiles/weapons/artillery.h"
#include "core/Hierarchy/EntityProfiles/weapons/rocket.h"
#include "core/Hierarchy/EntityProfiles/weapons/flare.h"
#include "core/Hierarchy/EntityProfiles/weapons/chaff.h"
#include "core/Hierarchy/EntityProfiles/weapons/sonobuoy.h"
#include "GUI/dialogwindowutils.h"

namespace {
constexpr const char* kProfileType   = "profile";
constexpr const char* kFolderType    = "folder";
constexpr const char* kOptionTag     = "option";
constexpr const char* kDatabaseCtx   = "database";
constexpr const char* kScenarioCtx   = "scenario";
constexpr const char* kRuntimeCtx    = "runtime";
}

static Weapon* createWeapon(const QString& typeName, Hierarchy* h)
{
    if (typeName == "Bomb")      return new Bomb(h);
    if (typeName == "Sonobuoy")  return new Sonobuoy(h);
    if (typeName == "Torpedo")   return new Torpedo(h);
    if (typeName == "Artillery") return new Artillery(h);
    if (typeName == "Rocket")    return new Rocket(h);
    if (typeName == "Flare")     return new Flare(h);
    if (typeName == "Chaff")     return new Chaff(h);
    return new Missile(h);
}

ContextMenu::ContextMenu(QWidget *parent)
    : QMenu(parent)
{
    setStyleSheet(ContextMenuStyles::ContextMenu);
}

void ContextMenu::setupMenu(QTreeWidgetItem *item)
{
    if (!item) return;
    clear();

    QVariantMap storedData = item->data(0, Qt::UserRole).toMap();
    QString type;

    if (storedData["type"].type() == QVariant::Map) {
        QVariantMap typeData = storedData["type"].toMap();
        if (typeData.contains("type") && typeData["type"].toString() == kOptionTag) {
            type = kProfileType;
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
    else if (type == kProfileType)   setupProfileMenu(storedData);
    else if (type == kFolderType)    setupFolderMenu(storedData);
    else if (type == "entity")       setupEntityMenu(storedData);
    else if (type == "component")    setupComponentMenu(storedData);
}

QString ContextMenu::getCurrentEditorContext() const
{
    MainWindow* mw = MainWindow::instance();
    if (!mw) return "";

    QWidget *cur = mw->stackedWidget->currentWidget();
    if (cur == mw->databaseEditor) return kDatabaseCtx;
    if (cur == mw->scenarioEditor) return kScenarioCtx;
    if (cur == mw->runtimeEditor)  return kRuntimeCtx;
    return "";
}

Hierarchy* ContextMenu::resolveHierarchyForContext(const QString& editorContext, bool isWeaponProfile) const
{
    MainWindow* mainWindow = MainWindow::instance();
    if (!mainWindow) return nullptr;
    Hierarchy* result = nullptr;
    if (isWeaponProfile) {
        if      (editorContext == kDatabaseCtx) result = mainWindow->getDatabaseHierarchy();
        else if (editorContext == kScenarioCtx && mainWindow->scenarioEditor) result = mainWindow->scenarioEditor->hierarchy;
        else if (editorContext == kRuntimeCtx  && mainWindow->runtimeEditor)  result = mainWindow->runtimeEditor->hierarchy;
    } else {
        result = mainWindow->getDatabaseHierarchy();
    }
    if (!result) result = mainWindow->getDatabaseHierarchy();
    return result;
}

QString ContextMenu::findParentProfileType(const QString& parentID) const
{
    if (!parentWidget()) return "";
    QTreeWidget* treeWidget = parentWidget()->findChild<QTreeWidget*>();
    if (!treeWidget) return "";

    QTreeWidgetItemIterator it(treeWidget);
    while (*it) {
        QVariantMap pData = (*it)->data(0, Qt::UserRole).toMap();
        if (pData["ID"].toString() != parentID) {
            ++it;
            continue;
        }
        // Case 1: parentID points directly at a profile node.
        if (pData["type"].type() == QVariant::Map) {
            QVariantMap typeData = pData["type"].toMap();
            if (typeData["type"].toString() == kOptionTag) {
                return typeData["value"].toString();
            }
            return "";
        }

        // Case 2: parentID points at a folder — walk up to its parent profile.
        if (pData["type"].toString() == kFolderType) {
            QString gpID = pData["parentId"].toString();
            QTreeWidgetItemIterator git(treeWidget);
            while (*git) {
                QVariantMap gpData = (*git)->data(0, Qt::UserRole).toMap();
                if (gpData["ID"].toString() == gpID) {
                    if (gpData["type"].type() == QVariant::Map) {
                        QVariantMap gpTypeData = gpData["type"].toMap();
                        if (gpTypeData["type"].toString() == kOptionTag) {
                            return gpTypeData["value"].toString();
                        }
                    }
                    break;
                }
                ++git;
            }
        }
        return "";
    }
    return "";
}

Weapon* ContextMenu::buildWeaponFromDialog(const AddWeaponDialog& dlg,
                                           Hierarchy* correctHierarchy,
                                           Hierarchy* dbHierarchy,
                                           const QString& parentID,
                                           bool fromDatabaseSelection) const
{
    QString weaponName = dlg.weaponName().trimmed();
    if (weaponName.isEmpty() || !correctHierarchy) return nullptr;

    QString typeName = dlg.weaponTypeStr();
    QString selectedId = dlg.selectedEntityId();

    if (fromDatabaseSelection) {

        if (selectedId.isEmpty() || !dbHierarchy) return nullptr;
        auto it = dbHierarchy->Weapons.find(selectedId.toStdString());
        if (it != dbHierarchy->Weapons.end() && it->second) {
            typeName = it->second->weaponTypeName();
        }
        Weapon* weapon = createWeapon(typeName, correctHierarchy);
        weapon->Name     = weaponName.toStdString();
        weapon->parentID = parentID.toStdString();
        if (it != dbHierarchy->Weapons.end() && it->second) {
            std::string savedId = weapon->ID, savedName = weapon->Name, savedParent = weapon->parentID;
            weapon->fromJson(it->second->toJson());
            weapon->ID = savedId; weapon->Name = savedName; weapon->parentID = savedParent;
        }
        return weapon;
    }

    Weapon* weapon = createWeapon(typeName, correctHierarchy);
    QJsonObject config = dlg.configJson();
    config["weaponTypeName"] = typeName;
    weapon->fromJson(config);
    weapon->Name     = weaponName.toStdString();
    weapon->parentID = parentID.toStdString();
    return weapon;
}

void ContextMenu::setupSubComponentMenu(const QVariantMap &data)
{
    QString ID       = data["ID"].toString();
    QString parentID = data["parentId"].toString();
    QString name     = data["name"].toString();

    QString subCompType = "";
    if (m_hierarchy) {
        auto it = m_hierarchy->Weapons.find(ID.toStdString());
        if (it != m_hierarchy->Weapons.end() && it->second) {
            subCompType = it->second->weaponTypeName();
        }
    }

    QAction *removeSubComponent = addAction(QIcon(":/icons/images/delete.png"), "Remove");
    QAction *rename             = addAction(QIcon(":/icons/images/rename.png"),  "Rename");

    QAction *editAction = nullptr;
    QString editorCtx = getCurrentEditorContext();

    if (!subCompType.isEmpty() && editorCtx != kDatabaseCtx) {
        editAction = addAction(QIcon(":/icons/images/edit.png"), "Edit");
        connect(editAction, &QAction::triggered, this, [=]() {
            emit editSubComponentRequested(parentID, ID, name, subCompType);
        });
    }

    connect(removeSubComponent, &QAction::triggered, this, [=]() {
        emit removeSubComponentRequested(parentID, ID, name);
    });

    connect(rename, &QAction::triggered, this, [=]() {
        QInputDialog dialog(this);
        makeDialogResizable(&dialog);
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

void ContextMenu::setupProfileMenu(const QVariantMap &data)
{
    QString ID           = data["ID"].toString();
    QString name         = data["name"].toString();
    QString specificType = data["type"].toMap()["value"].toString();
    QString lowerType    = specificType.toLower();

    bool hasClipboard = !m_copiedItems.isEmpty() || HierarchyConnector::instance()->hasCopiedItem();

    QAction *paste = nullptr;
    if (hasClipboard) {
        paste = addAction(QIcon(":/icons/images/paste.png"), "Paste");
    }
    QAction *addFolder = addAction(QIcon(":/icons/images/add.png"), "Add Folder");

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
    QAction *addEntity = addAction(QIcon(":/icons/images/add.png"), addEntityText);
    connect(addFolder, &QAction::triggered, this, [=]() {
        QInputDialog dialog(this);
        makeDialogResizable(&dialog);
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

    connect(addEntity, &QAction::triggered, this, [=]() {
        bool isSensorProfile = (lowerType == "sensor");
        bool isWeaponProfile = (lowerType == "weapon");
        QString editorContext = getCurrentEditorContext();
        Hierarchy* correctHierarchy = resolveHierarchyForContext(editorContext, isWeaponProfile);
        MainWindow* mainWindow = MainWindow::instance();
        // associated database is loaded in the Database Editor.
        if (editorContext == kScenarioCtx || editorContext == kRuntimeCtx) {
            Hierarchy* targetHier = nullptr;
            if (editorContext == kScenarioCtx && mainWindow && mainWindow->scenarioEditor)
                targetHier = mainWindow->scenarioEditor->hierarchy;
            else if (editorContext == kRuntimeCtx && mainWindow && mainWindow->runtimeEditor)
                targetHier = mainWindow->runtimeEditor->hierarchy;
            if (targetHier
                && !HierarchyConnector::instance()->confirmAndLoadAssociatedDatabase(targetHier, this)) {
                return; // user cancelled
            }
        }

        // ── Weapon Profile ───────────────────────────────────────────
        if (isWeaponProfile) {
            if (!correctHierarchy) { QMessageBox::critical(this, "Error", "Hierarchy not available"); return; }
            Hierarchy* dbHierarchy = mainWindow ? mainWindow->getDatabaseHierarchy() : nullptr;

            bool isDbContext = (editorContext == kDatabaseCtx);
            AddWeaponDialog dlg(parentWidget(), dbHierarchy, isDbContext);
            dlg.setStyleSheet(ContextMenuStyles::AddItemDialog);
            if (dlg.exec() != QDialog::Accepted) return;
            if (dlg.weaponName().trimmed().isEmpty()) {
                QMessageBox::warning(this, "Validation Error", "Weapon name cannot be empty");
                return;
            }
            if (!isDbContext && dlg.selectedEntityId().isEmpty()) {
                QMessageBox::warning(this, "Validation Error", "Please select a weapon from the database");
                return;
            }

            try {
                Weapon* weapon = buildWeaponFromDialog(dlg, correctHierarchy, dbHierarchy, ID, !isDbContext);
                if (!weapon) {
                    QMessageBox::critical(this, "Error", "Failed to build weapon from dialog input");
                    return;
                }
                auto profIt = correctHierarchy->ProfileCategories.find(ID.toStdString());
                if (profIt == correctHierarchy->ProfileCategories.end() || !profIt->second) {
                    QMessageBox::critical(this, "Error", "Weapon profile not found in hierarchy");
                    delete weapon;
                    return;
                }

                profIt->second->addEntityWithObject(weapon);
                weapon->syncComponentsFromWeaponData();
            } catch (const std::exception& e) {
                QMessageBox::critical(this, "Error", QString("Failed to create weapon: %1").arg(e.what()));
            }
            return;
        }

        // ── All Other Profiles ───────────────────────────────────────
        AddItemDialog dialog(AddItemDialog::EntityType, specificType,
                             AddItemDialog::NormalMode, correctHierarchy, this, editorContext);
        dialog.setStyleSheet(ContextMenuStyles::AddItemDialog);
        dialog.setWindowTitle(specificTypeNames.contains(lowerType)
                                  ? "Add " + specificTypeNames[lowerType]
                                  : "Add Entity");

        if (dialog.exec() == QDialog::Accepted && !dialog.getName().isEmpty()) {
            bool isProfileParent = true;
            if (isSensorProfile) {
                emit addEntityRequested(ID, dialog.getName(), isProfileParent, dialog.getComponents(), &dialog);
                return;
            }

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
    });
    if (paste) {
        connect(paste, &QAction::triggered, this, [=]() {
            if (!m_copiedItems.isEmpty()) emit pasteItemsRequested(data, m_copiedItems);
            else                          emit pasteItemRequested(data);
        });
    }
}

void ContextMenu::setupFolderMenu(const QVariantMap &data)
{
    QString ID       = data["ID"].toString();
    QString parentID = data["parentId"].toString();
    QString name     = data["name"].toString();

    // FIX (#6): now uses the shared helper instead of an inline tree-walk.
    QString parentProfileType = findParentProfileType(parentID);

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

    QAction *rename       = addAction(QIcon(":/icons/images/rename.png"),   "Rename");
    QAction *paste        = addAction(QIcon(":/icons/images/paste.png"),    "Paste");
    QAction *addFolder    = addAction(QIcon(":/icons/images/add.png"),      "Add Folder");
    QAction *addEntity    = addAction(QIcon(":/icons/images/add.png"),      addEntityLabel);
    QAction *deleteFolder = addAction(QIcon(":/icons/images/delete.png"),   "Delete Folder");

    connect(addFolder, &QAction::triggered, this, [=]() {
        QInputDialog dialog(this);
        makeDialogResizable(&dialog);
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

    connect(addEntity, &QAction::triggered, this, [=]() {
        QString editorContext = getCurrentEditorContext();
        bool isWeaponProfile = (parentProfileType == "Weapon");
        Hierarchy* correctHierarchy = resolveHierarchyForContext(editorContext, isWeaponProfile);
        if (!correctHierarchy) return;
        if (editorContext == kScenarioCtx || editorContext == kRuntimeCtx) {
            MainWindow* mw = MainWindow::instance();
            Hierarchy* targetHier = nullptr;
            if (editorContext == kScenarioCtx && mw && mw->scenarioEditor)
                targetHier = mw->scenarioEditor->hierarchy;
            else if (editorContext == kRuntimeCtx && mw && mw->runtimeEditor)
                targetHier = mw->runtimeEditor->hierarchy;
            if (targetHier
                && !HierarchyConnector::instance()->confirmAndLoadAssociatedDatabase(targetHier, this)) {
                return;
            }
        }
        if (isWeaponProfile) {
            MainWindow* mainWindow = MainWindow::instance();
            Hierarchy* dbHierarchy = mainWindow ? mainWindow->getDatabaseHierarchy() : nullptr;
            bool isDbContext = (editorContext == kDatabaseCtx);

            AddWeaponDialog dlg(parentWidget(), dbHierarchy, isDbContext);
            dlg.setStyleSheet(ContextMenuStyles::AddItemDialog);
            if (dlg.exec() != QDialog::Accepted) return;
            if (dlg.weaponName().trimmed().isEmpty()) return;
            Weapon* weapon = buildWeaponFromDialog(dlg, correctHierarchy, dbHierarchy, ID, !isDbContext);
            if (!weapon) return;
            auto folderIt = correctHierarchy->Folders.find(ID.toStdString());
            if (folderIt == correctHierarchy->Folders.end() || !folderIt->second) {
                QMessageBox::critical(this, "Error", "Target folder not found in hierarchy");
                delete weapon;
                return;
            }
            folderIt->second->addEntityWithObject(weapon);
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

    connect(deleteFolder, &QAction::triggered, this, [=]() {
        QMessageBox msgBox(this);
        msgBox.setStyleSheet(ContextMenuStyles::MessageBox);
        msgBox.setWindowTitle("Delete Folder");
        msgBox.setText(QString("Are you sure you want to delete folder '%1'?").arg(name));
        msgBox.setIcon(QMessageBox::Question);
        msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
        if (msgBox.exec() == QMessageBox::Yes) emit removeFolderRequested(parentID, ID, false);
    });

    connect(paste, &QAction::triggered, this, [=]() {
        if (!m_copiedItems.isEmpty()) emit pasteItemsRequested(data, m_copiedItems);
        else                          emit pasteItemRequested(data);
    });

    connect(rename, &QAction::triggered, this, [=]() {
        QInputDialog dialog(this);
        makeDialogResizable(&dialog);
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
    bool isPlatformEntity = (findParentProfileType(parentID) == "Platform");
    QAction *rename = addAction(QIcon(":/icons/images/rename.png"), "Rename");
    bool isFormation = false;
    if (m_hierarchy) {
        auto it = m_hierarchy->Entities.find(ID.toStdString());
        if (it != m_hierarchy->Entities.end()) {
            isFormation = dynamic_cast<Formation*>(it->second) != nullptr;
        }
    }
    QAction *copy = nullptr;
    if (!isFormation) {
        copy = addAction(QIcon(":/icons/images/copy.png"), "Copy");
    }

    QAction *setActive   = nullptr;
    QAction *setInactive = nullptr;
    QMenu *addComponentMenu = nullptr;
    QAction *addWeapon = nullptr, *addSensor = nullptr, *addIFF = nullptr, *addRadio = nullptr;

    if (isPlatformEntity) {
        setActive   = addAction(QIcon(":/icons/images/enable.png"),  "Set Active");
        setInactive = addAction(QIcon(":/icons/images/disable.png"), "Set Inactive");

        addComponentMenu = addMenu(QIcon(":/icons/images/add.png"), "Add");
        addComponentMenu->setStyleSheet(styleSheet());
        addWeapon = addComponentMenu->addAction(QIcon(":/icons/images/bio-weapon.png"),      "Add Weapon");
        addSensor = addComponentMenu->addAction(QIcon(":/icons/images/database (1).png"),    "Add Sensor");
        addIFF    = addComponentMenu->addAction(QIcon(":/icons/images/identification.png"),  "Add IFF");
        addRadio  = addComponentMenu->addAction(QIcon(":/icons/images/radio.png"),           "Add Radio");

        QMenu *setTeamMenu = addMenu(QIcon(":/icons/images/seteam.png"), "Set Team");
        setTeamMenu->setStyleSheet(styleSheet());
        const QStringList teams = {"RedTeam", "BlueTeam", "GreenTeam", "YellowTeam",
                                   "GreyTeam", "AlphaTeam", "BetaTeam", "GammaTeam"};
        for (const QString& team : teams) {
            QAction *teamAction = setTeamMenu->addAction(team);
            connect(teamAction, &QAction::triggered, this, [=]() {
                emit addTeamToEntityRequested(data, team);
            });
        }
        if (getCurrentEditorContext() == kDatabaseCtx) {
            QMenu *setCategoryMenu = addMenu(QIcon(":/icons/images/set.png"), "Set Category");
            setCategoryMenu->setStyleSheet(styleSheet());

            QMap<QString, QStringList> categoryMap = {
                {"Air",    QStringList{"Aircraft", "Helicopter", "UAV", "FighterAircraft", "TransportAircraft"}},
                {"Ground", QStringList{"Tank", "GroundRadar", "Human", "BattleVehicle", "MilitaryGroundVehicle", "TransportVehicle"}},
                {"Marine", QStringList{"Ship", "Frigate", "Submarine", "AircraftCarrier", "GuidedMissileCruiser", "Destroyer", "SmallBoat"}}
            };

            for (auto it = categoryMap.begin(); it != categoryMap.end(); ++it) {
                const QString category    = it.key();
                const QStringList subCats = it.value();

                QMenu* subCatMenu = setCategoryMenu->addMenu(category);
                subCatMenu->setStyleSheet(styleSheet());

                for (const QString& subCat : subCats) {
                    const QString subCategory = subCat;
                    QAction* subCatAction = subCatMenu->addAction(subCategory);
                    connect(subCatAction, &QAction::triggered, this, [=]() {
                        emit setCategoryToEntityRequested(data, category, subCategory);
                    });
                }
            }
        }
    }

    QAction *editEntity = nullptr;
    QString editorCtx = getCurrentEditorContext();
    if (isPlatformEntity && editorCtx != kDatabaseCtx) {
        editEntity = addAction(QIcon(":/icons/images/edit.png"), "Edit Dynamic Model");
        connect(editEntity, &QAction::triggered, this, [=]() {
            emit editEntityRequested(data);
        });
    }

    QAction *deleteEntity = addAction(QIcon(":/icons/images/delete.png"), "Delete");

    connect(rename, &QAction::triggered, this, [=]() {
        QInputDialog dialog(this);
        makeDialogResizable(&dialog);
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

    if (copy) {
        connect(copy, &QAction::triggered, this, [=]() { emit copyItemRequested(data); });
    }

    connect(deleteEntity, &QAction::triggered, this, [=]() {
        QMessageBox msgBox(this);
        msgBox.setStyleSheet(ContextMenuStyles::MessageBox);
        msgBox.setWindowTitle("Delete");
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
    // --- Leave Formation (ally only; mothership par mat dikhao) ---
    if (m_hierarchy && isPlatformEntity && getCurrentEditorContext() != kDatabaseCtx) {
        Platform* pl = nullptr;
        auto eit = m_hierarchy->Entities.find(ID.toStdString());
        if (eit != m_hierarchy->Entities.end())
            pl = dynamic_cast<Platform*>(eit->second);

        Formation* parentForm = nullptr;
        if (pl)
            parentForm = pl->dynamicModel.formation;

        // Fallback: scan formations agar pointer null ho
        if (!parentForm) {
            for (const auto& pair : m_hierarchy->Entities) {
                Formation* f = dynamic_cast<Formation*>(pair.second);
                if (f && f->isAllyEntity(ID)) {
                    parentForm = f;
                    break;
                }
            }
        }

        if (parentForm && parentForm->isAllyEntity(ID) &&
            !parentForm->isMothershipEntity(ID)) {
            addSeparator();
            QAction* leaveAct = addAction(
                QIcon(":/icons/images/delete.png"), "Leave Formation");
            const QString formId = QString::fromStdString(parentForm->ID);
            connect(leaveAct, &QAction::triggered, this, [=]() {
                emit removeEntityFromFormationRequested(formId, ID);
            });
        }
    }
    // Formation actions —
    if (m_hierarchy) {
        auto it = m_hierarchy->Entities.find(ID.toStdString());
        if (it != m_hierarchy->Entities.end()) {
            Formation* formation = dynamic_cast<Formation*>(it->second);
            if (formation) {
                QString editorCtx = getCurrentEditorContext();

                // Database Editor mein dono actions hide
                if (editorCtx != "database") {
                    addSeparator();

                    QAction* selectWithEntityAction = addAction(
                            QIcon(":/icons/images/selection.png"),"Select with Entity");
                    connect(selectWithEntityAction, &QAction::triggered, this, [this, ID]() {
                        emit selectFormationWithEntityRequested(ID);
                    });

                    QAction* changeMotherAction = addAction(
                        QIcon(":/icons/images/edit.png"), "Change Mothership");
                    connect(changeMotherAction, &QAction::triggered, this, [this, ID]() {
                        emit changeMothershipRequested(ID);
                    });
                    QAction* addEntityToFormAction = addAction(
                        QIcon(":/icons/images/add.png"), "Add Entity…");
                    connect(addEntityToFormAction, &QAction::triggered, this, [this, ID]() {
                        emit addEntityToFormationRequested(ID);
                    });
                    const bool active = formation->isFormationActive();

                    if (active) {
                        QAction* breakAct = addAction(
                            QIcon(":/icons/images/disable.png"), "Break Formation");
                        connect(breakAct, &QAction::triggered, this, [this, ID]() {
                            emit breakFormationRequested(ID);
                        });
                    } else {
                        // Sirf tab dikhao jab slots mein entities assigned hain
                        bool hasMembers = false;
                        if (formation->formationPositions) {
                            for (const auto& pair : *formation->formationPositions) {
                                if (pair.second && pair.second->entity) {
                                    hasMembers = true;
                                    break;
                                }
                            }
                        }
                        if (hasMembers) {
                            QAction* resumeAct = addAction(
                                QIcon(":/icons/images/enable.png"), "Resume Formation");
                            connect(resumeAct, &QAction::triggered, this, [this, ID]() {
                                emit resumeFormationRequested(ID);
                            });
                }
            }
        }
    }
}
    }
}
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
            Hierarchy* dbHierarchy = mainWindow ? mainWindow->getDatabaseHierarchy() : nullptr;
            AddWeaponDialog dlg(parentWidget(), dbHierarchy, true);
            dlg.setStyleSheet(ContextMenuStyles::AddItemDialog);
            if (dlg.exec() != QDialog::Accepted) return;
            QString weaponName = dlg.weaponName().trimmed();
            if (weaponName.isEmpty()) return;
            emit addComponentRequested(parentID, "weapons", weaponName, dlg.weaponTypeStr(), "");
            if (correctHierarchy->Entities.count(parentID.toStdString())) {
                Entity* ent = (correctHierarchy->Entities)[parentID.toStdString()];
                Platform* plf = dynamic_cast<Platform*>(ent);
                if (plf /*&& plf->weapons*/) {
                    for (auto& [wid, w] : *plf->weapons.weapons) {
                        if (QString::fromStdString(w->Name) == weaponName) {
                            QJsonObject cfg = dlg.configJson();
                            cfg["weaponTypeName"] = dlg.weaponTypeStr();
                            w->fromJson(cfg);
                            w->Name = weaponName.toStdString();
                            w->syncComponentsFromWeaponData();
                            plf->weapons.updateSubComponent(wid, w->toJson());
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
            {
                QString editorContext = getCurrentEditorContext();
                if (editorContext == kScenarioCtx || editorContext == kRuntimeCtx) {
                    MainWindow* mw = MainWindow::instance();
                    Hierarchy* targetHier = nullptr;
                    if (editorContext == kScenarioCtx && mw && mw->scenarioEditor)
                        targetHier = mw->scenarioEditor->hierarchy;
                    else if (editorContext == kRuntimeCtx && mw && mw->runtimeEditor)
                        targetHier = mw->runtimeEditor->hierarchy;
                    if (targetHier
                        && !HierarchyConnector::instance()->confirmAndLoadAssociatedDatabase(targetHier, this)) {
                        return;
                    }
                }
            }
            QString componentType = name.toLower();
            AddItemDialog::DialogMode mode = AddItemDialog::NormalMode;
            if (componentType == "sensors") {
                mode = AddItemDialog::ComponentSensorMode;
            } else if (componentType == "iffs") {
                mode = AddItemDialog::ComponentIFFMode;
            } else if (componentType == "radios") {
                mode = AddItemDialog::ComponentRadioMode;
            }
            MainWindow* mw = MainWindow::instance();
            Hierarchy* dbHierarchy = mw ? mw->getDatabaseHierarchy() : nullptr;
            AddItemDialog dialog(AddItemDialog::EntityType,
                                 componentType,
                                 mode,
                                 dbHierarchy,
                                 this);
            if (dialog.exec() == QDialog::Accepted && !dialog.getName().isEmpty()) {
                QString profileId  = dialog.getProfileId();
                QString sensorType;
                if (componentType == "sensors") {
                    sensorType = dialog.getSensorType();
                } else if (componentType == "radios") {
                    sensorType = dialog.getRadioType();
                } else if (componentType == "iffs") {
                    sensorType = dialog.getIFFType();
                }
                emit addComponentRequested(ID, componentType, dialog.getName(), sensorType, profileId);
            }
        });
    }
}

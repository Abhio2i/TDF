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

#include "additemdialog.h"
#include "additemdialog-styles.h"
#include "GUI/Hierarchytree/customtrajectorydialog.h"
#include "core/Hierarchy/hierarchy.h"
#include "core/Hierarchy/EntityProfiles/radio.h"
#include "core/Hierarchy/EntityProfiles/sensor.h"
#include "core/Hierarchy/EntityProfiles/iff.h"
#include "core/Hierarchy/EntityProfiles/fixedpoints.h"
#include "core/Hierarchy/EntityProfiles/platform.h"
#include "core/Hierarchy/entity.h"
#include "qabstractitemview.h"
#include "qpushbutton.h"
#include "GUI/mainwindow.h"
#include <QSortFilterProxyModel>
#include <QCheckBox>
#include <QComboBox>
#include <QCompleter>
#include <QStringListModel>
#include <QDialogButtonBox>
#include <QFrame>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QScrollArea>
#include <QScrollBar>
#include <QSpinBox>
#include <QValidator>
#include <QVBoxLayout>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonParseError>
#include <QApplication>
#include <QMap>
#include <QPointF>
#include <QString>
#include "Setup.h"
#include <QMessageBox>
#include <QStandardItemModel>
#include <QStandardItem>

QMap<QString, QPointF> indianCities = {
};
// %%% String Utility Functions %%%


QString demangleComponentName(const std::string& mangledName) {
    QString name = QString::fromStdString(mangledName);
    while (!name.isEmpty() && name[0].isDigit()) {
        name.remove(0, 1);
    }
    // Convert to proper capitalization
    if (!name.isEmpty()) {
        bool isAllCaps = true;
        for (int i = 0; i < name.length(); ++i) {
            if (name[i].isLetter() && name[i].isLower()) {
                isAllCaps = false;
                break;
            }
        }
        if (!isAllCaps) {
            name[0] = name[0].toUpper();
        }
    }
    return name;
}

/* Convert string to camelCase format */
QString toCamelCase(const QString& input) {
    if (input.isEmpty()) return input;
    QString result = input;
    if (!result.isEmpty()) {
        result[0] = result[0].toLower();
    }
    for (int i = 1; i < result.size(); ++i) {
        if (result[i] == 'D' && i > 0 && result[i-1].isDigit()) {
            result[i] = result[i].toLower();
        }
        else if (result[i].isUpper() && result[i-1].isLower()) {

        }
        else if (result[i].isUpper() && result[i-1].isUpper()) {
            result[i] = result[i].toLower();
        }
    }
    return result;
}

/* Get default item name based on dialog type */
QString getDefaultName(AddItemDialog::DialogType type) {
    return (type == AddItemDialog::EntityType) ? "Entity" : "Folder";
}

// %%% Constructor %%%

AddItemDialog::AddItemDialog(DialogType type,
                             const QString &specificType,
                             DialogMode dialogMode,
                             Hierarchy* hierarchy,
                             QWidget *parent,
                             const QString& editorContext)
    : QDialog(parent),
    customDialog(this),
    nameLineEdit(nullptr),
    numberLineEdit(nullptr),
    sensorTypeComboBox(nullptr),
    profileComboBox(nullptr),
    scCheckBox(nullptr),
    scOptionsGroup(nullptr),
    scTypeComboBox(nullptr),
    rangeLineEdit(nullptr),
    minRadioRangeSpinBox(nullptr),
    maxRadioRangeSpinBox(nullptr),
    minRadarRangeSpinBox(nullptr),
    maxRadarRangeSpinBox(nullptr),
    trajectoryComboBox(nullptr),
    minPlaneSpeedSpinBox(nullptr),
    maxPlaneSpeedSpinBox(nullptr),
    minTurnRadiusSpinBox(nullptr),
    maxTurnRadiusSpinBox(nullptr),
    teamSelectComboBox(nullptr),
    entitySearchLineEdit(nullptr),
    profileFilterComboBox(nullptr),
    entityCompleter(nullptr),
    specificType(specificType),
    m_dialogMode(dialogMode),
    m_hierarchy(hierarchy),
    m_editorContext(editorContext)
{
    setStyleSheet(AddItemDialogStyles::Dialog);
    QString cityJsonPath = TDFManager::instance()->getCityJsonPath();
    QFile file(cityJsonPath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return;
    }
    QByteArray jsonData = file.readAll();
    file.close();
    QJsonParseError parseError;
    QJsonDocument jsonDoc = QJsonDocument::fromJson(jsonData, &parseError);
    if (parseError.error != QJsonParseError::NoError) {
        return;
    }
    if (jsonDoc.isObject()) {
        QJsonObject jsonObj = jsonDoc.object();
        if (jsonObj.contains("indian_cities") && jsonObj["indian_cities"].isArray()) {
            QJsonArray indian_cities = jsonObj["indian_cities"].toArray();
            indianCities.clear();
            for (const QJsonValue &value : indian_cities) {
                QJsonObject cityObj = value.toObject();
                QString name = cityObj["name"].toString();
                double lat = cityObj["lat"].toDouble();
                double lon = cityObj["lon"].toDouble();
                indianCities.insert(name, QPointF(lat, lon));
            }
        }
    }
    m_profileContext = determineProfileContext(specificType, dialogMode, editorContext);
    setupUI(type);
}

// %%% Helper Methods %%%

/* Create min-max spinbox pair with labels and units */
QHBoxLayout* AddItemDialog::createMinMaxSpinBoxPair(const QString& label,
                                                    QSpinBox*& minSpinBox,
                                                    QSpinBox*& maxSpinBox,
                                                    int minDefault,
                                                    int maxDefault,
                                                    int minRange,
                                                    int maxRange,
                                                    QString unit)
{
    QHBoxLayout *layout = new QHBoxLayout();
    QLabel *labelWidget = new QLabel(label + ":", this);
    labelWidget->setStyleSheet("color: white;");
    layout->addWidget(labelWidget);

    minSpinBox = new QSpinBox(this);
    minSpinBox->setRange(minRange, maxRange);
    minSpinBox->setValue(minDefault);
    minSpinBox->setSuffix(unit);
    minSpinBox->setStyleSheet(AddItemDialogStyles::SpinBox);

    QLabel *minLabel = new QLabel("Min:", this);
    minLabel->setStyleSheet("color: white;");
    layout->addWidget(minLabel);
    layout->addWidget(minSpinBox);

    maxSpinBox = new QSpinBox(this);
    maxSpinBox->setRange(minRange, maxRange);
    maxSpinBox->setValue(maxDefault);
    maxSpinBox->setSuffix(unit);
    maxSpinBox->setStyleSheet(AddItemDialogStyles::SpinBox);

    QLabel *maxLabel = new QLabel("Max:", this);
    maxLabel->setStyleSheet("color: white;");
    layout->addWidget(maxLabel);
    layout->addWidget(maxSpinBox);

    layout->addStretch();
    return layout;
}

void AddItemDialog::setupScSection()
{
    scCheckBox = new QCheckBox("Scenarioconfig", this);
    scCheckBox->setChecked(false);
    connect(scCheckBox, &QCheckBox::stateChanged,
            this, &AddItemDialog::onScCheckBoxStateChanged);

    // Create options group
    scOptionsGroup = new QGroupBox("Scenarioconfig Options", this);
    scOptionsGroup->setStyleSheet(AddItemDialogStyles::GroupBox);
    QVBoxLayout *groupLayout = new QVBoxLayout();

    // Type selection
    QHBoxLayout *typeLayout = new QHBoxLayout();

    QLabel *typeLabel = new QLabel("Type:", this);
    typeLabel->setStyleSheet("color: white;");
    typeLayout->addWidget(typeLabel);

    scTypeComboBox = new QComboBox(this);
    scTypeComboBox->addItems({"Spread", "Circle"});
    scTypeComboBox->setCurrentText("Spread");
    typeLayout->addWidget(scTypeComboBox);
    typeLayout->addStretch();
    groupLayout->addLayout(typeLayout);

    QHBoxLayout *cityLayout = new QHBoxLayout();

    QLabel *cityLabel = new QLabel("Center:", this);
    cityLabel->setStyleSheet("color: white;");
    cityLayout->addWidget(cityLabel);

    cityComboBox = new QComboBox(this);
    cityComboBox->setEditable(true);
    cityComboBox->setInsertPolicy(QComboBox::NoInsert);
    cityComboBox->setCurrentText("Bengaluru");

    QStringList allCityNames;
    for (auto it = indianCities.begin(); it != indianCities.end(); ++it) {
        allCityNames.append(it.key());
    }
    allCityNames.sort(Qt::CaseInsensitive);
    QStringListModel *cityModel = new QStringListModel(allCityNames, this);
    QSortFilterProxyModel *proxyModel = new QSortFilterProxyModel(this);
    proxyModel->setSourceModel(cityModel);
    proxyModel->setFilterCaseSensitivity(Qt::CaseInsensitive);
    proxyModel->setFilterFixedString("");
    cityComboBox->setModel(proxyModel);
    QCompleter *cityCompleter = new QCompleter(proxyModel, this);
    cityCompleter->setCaseSensitivity(Qt::CaseInsensitive);
    cityCompleter->setCompletionMode(QCompleter::PopupCompletion);
    cityCompleter->setMaxVisibleItems(15);
    cityCompleter->setFilterMode(Qt::MatchStartsWith);
    cityComboBox->setCompleter(cityCompleter);
    cityComboBox->lineEdit()->setPlaceholderText("Search city...");
    cityComboBox->setCurrentIndex(-1);
    cityComboBox->lineEdit()->clear();
    // Style completer popup
    if (cityCompleter->popup()) {
        cityCompleter->popup()->setStyleSheet(AddItemDialogStyles::CompleterPopup);
    }
    QAction *showCitiesAction = new QAction(this);
    cityComboBox->lineEdit()->addAction(showCitiesAction, QLineEdit::TrailingPosition);
    connect(showCitiesAction, &QAction::triggered, this, [=]() {
        cityComboBox->lineEdit()->clear();
        cityComboBox->lineEdit()->setPlaceholderText("Search city...");
        proxyModel->setFilterFixedString("");
        cityComboBox->showPopup();
    });
    connect(cityCompleter, QOverload<const QString &>::of(&QCompleter::activated),
            this, [=](const QString &text) {
                cityComboBox->setCurrentText(text);
            });
    connect(cityComboBox->lineEdit(), &QLineEdit::textChanged, this, [=](const QString &text) {
        QString trimmedText = text.trimmed();
        if (trimmedText.isEmpty()) {
            cityComboBox->lineEdit()->setPlaceholderText("Search city...");
            proxyModel->setFilterFixedString("");
            cityComboBox->setCurrentIndex(-1);
            cityComboBox->clearEditText();
        } else {
            cityComboBox->lineEdit()->setPlaceholderText("");
            proxyModel->setFilterFixedString(trimmedText);
        }
    });
    connect(cityComboBox->lineEdit(), &QLineEdit::editingFinished, this, [=]() {
        QString currentText = cityComboBox->lineEdit()->text().trimmed();
        if (currentText.isEmpty()) {
            cityComboBox->setCurrentIndex(-1);
            cityComboBox->clearEditText();
            proxyModel->setFilterFixedString("");
            cityComboBox->lineEdit()->setPlaceholderText("Search city...");
            return;
        }
        QString matchedCity;
        bool foundExact = false;
        for (const QString &city : allCityNames) {
            if (city.compare(currentText, Qt::CaseInsensitive) == 0) {
                matchedCity = city;
                foundExact = true;
                break;
            }
        }
        if (foundExact) {
            cityComboBox->setCurrentText(matchedCity);
        } else {
            cityComboBox->setCurrentIndex(-1);
            cityComboBox->clearEditText();
            proxyModel->setFilterFixedString("");
            cityComboBox->lineEdit()->setPlaceholderText("Search city...");
        }
    });
    cityLayout->addWidget(cityComboBox);
    groupLayout->addLayout(cityLayout);
    QHBoxLayout *rangeLayout = new QHBoxLayout();
    QLabel *rangeLabel = new QLabel("Range:", this);
    rangeLabel->setStyleSheet("color: white;");
    rangeLayout->addWidget(rangeLabel);
    rangeLineEdit = new QLineEdit(this);
    rangeLineEdit->setText("100");
    rangeLineEdit->setPlaceholderText("Enter integer value");
    rangeLineEdit->setValidator(new QIntValidator(0, 1000000, this));
    rangeLayout->addWidget(rangeLineEdit);
    QLabel *kmLabel = new QLabel(" km", this);
    kmLabel->setStyleSheet("color: white;");
    rangeLayout->addWidget(kmLabel);
    groupLayout->addLayout(rangeLayout);
    // Add separator
    QFrame *separator1 = new QFrame(this);
    separator1->setFrameShape(QFrame::HLine);
    separator1->setStyleSheet(AddItemDialogStyles::Frame);
    groupLayout->addWidget(separator1);
    // Radio range parameters
    groupLayout->addLayout(createMinMaxSpinBoxPair("Radio Range",
                                                   minRadioRangeSpinBox, maxRadioRangeSpinBox,
                                                   10, 100, 1, 10000));
    // Radar range parameters
    groupLayout->addLayout(createMinMaxSpinBoxPair("Radar Range",
                                                   minRadarRangeSpinBox, maxRadarRangeSpinBox,
                                                   50, 500, 1, 10000));
    // Add separator
    QFrame *separator2 = new QFrame(this);
    separator2->setFrameShape(QFrame::HLine);
    separator2->setStyleSheet(AddItemDialogStyles::Frame);
    groupLayout->addWidget(separator2);
    QHBoxLayout *trajectoryLayout = new QHBoxLayout();
    QLabel *trajLabel = new QLabel("Trajectory:", this);
    trajLabel->setStyleSheet("color: white;");
    trajectoryLayout->addWidget(trajLabel);
    QPushButton *trajectoryButton = new QPushButton("Configure Trajectory", this);
    trajectoryButton->setStyleSheet(AddItemDialogStyles::ButtonBox);
    trajectoryButton->setToolTip("Configure custom trajectory");
    connect(trajectoryButton, &QPushButton::clicked, this, [=]() {
        customDialog.exec();
    });
    trajectoryLayout->addWidget(trajectoryButton);
    trajectoryLayout->addStretch();
    groupLayout->addLayout(trajectoryLayout);
    // Add separator
    QFrame *separator3 = new QFrame(this);
    separator3->setFrameShape(QFrame::HLine);
    separator3->setStyleSheet(AddItemDialogStyles::Frame);
    groupLayout->addWidget(separator3);
    // Plane speed parameters
    QHBoxLayout *speedLayout = new QHBoxLayout();
    QLabel *speedLabel = new QLabel("Plane Speed:", this);
    speedLabel->setStyleSheet("color: white;");
    speedLayout->addWidget(speedLabel);
    minPlaneSpeedSpinBox = new QSpinBox(this);
    minPlaneSpeedSpinBox->setRange(100, 1000);
    minPlaneSpeedSpinBox->setValue(800);
    minPlaneSpeedSpinBox->setSuffix(" km/h");
    minPlaneSpeedSpinBox->setStyleSheet(AddItemDialogStyles::SpinBox);
    QLabel *minLabel = new QLabel("Min:", this);
    minLabel->setStyleSheet("color: white;");
    speedLayout->addWidget(minLabel);
    speedLayout->addWidget(minPlaneSpeedSpinBox);

    maxPlaneSpeedSpinBox = new QSpinBox(this);
    maxPlaneSpeedSpinBox->setRange(0, 8000);
    maxPlaneSpeedSpinBox->setValue(5000);
    maxPlaneSpeedSpinBox->setSuffix(" km/h");
    maxPlaneSpeedSpinBox->setStyleSheet(AddItemDialogStyles::SpinBox);

    QLabel *maxLabel = new QLabel("Max:", this);
    maxLabel->setStyleSheet("color: white;");
    speedLayout->addWidget(maxLabel);
    speedLayout->addWidget(maxPlaneSpeedSpinBox);
    speedLayout->addStretch();
    groupLayout->addLayout(speedLayout);

    groupLayout->addLayout(createMinMaxSpinBoxPair("Turn Rate",
                                                   minTurnRadiusSpinBox, maxTurnRadiusSpinBox,
                                                   25, 50, 1, 360, " deg/s"));
    groupLayout->addSpacing(10);
    scOptionsGroup->setLayout(groupLayout);
    scOptionsGroup->setVisible(false);
}

/* Handle scenario config checkbox state change */
void AddItemDialog::onScCheckBoxStateChanged(int state)
{
    bool isChecked = (state == Qt::Checked);
    if (scOptionsGroup) {
        scOptionsGroup->setVisible(isChecked);
    }
}

void AddItemDialog::populateEntityProfiles(const QString &profileTypeFilter,
                                           const QString &categoryFilter){
    if (!entitySearchLineEdit) {
        return;
    }

    entityMap.clear();
    QStringList entityNames;

    if (!m_hierarchy) {
        return;
    }
    QStringList profileTypesToShow;
    QString filter = profileTypeFilter.trimmed();
    bool showAllEntities = (filter.isEmpty() || filter == "All Profiles");
    if (filter == "All Profiles" || filter.isEmpty()) {
        if (!m_profileContext.isEmpty()) {
            profileTypesToShow.append(m_profileContext);
        }
    }
    else {
        profileTypesToShow.append(filter);
    }
    bool showFromAllProfiles = profileTypesToShow.isEmpty();
    for (const auto& [profileId, profile] : m_hierarchy->ProfileCategories)
    {
        if (!profile) continue;
        QString profileName = QString::fromStdString(profile->Name);
        bool includeThisProfile = showFromAllProfiles;

        if (!showFromAllProfiles) {
            includeThisProfile = profileTypesToShow.contains(profileName);
        }
        if (!includeThisProfile) continue;
        for (const auto& [entityId, entity] : profile->Entities)
        {
            if (!entity) continue;
            QString eName = QString::fromStdString(entity->Name).trimmed();
            if (eName.isEmpty()) continue;
            QString displayName;
            // ✅ FIXED CODE:
            if (profileName == "Platform") {
                QString categoryValue = QString::fromStdString(
                    entity->CategoryNames[static_cast<int>(entity->category)]
                    );

                QJsonObject entityJson = entity->toJson();
                QString subCatValue = "";
                if (entityJson.contains("SubCategory") && entityJson["SubCategory"].isObject()) {
                    subCatValue = entityJson["SubCategory"].toObject()["value"].toString();
                }

                if (!categoryFilter.isEmpty() && categoryFilter != "All") {
                    QStringList allSubCats = {"Aircraft", "Helicopter", "UAV",
                                              "Tank", "GroundRadar", "Human",
                                              "Ship", "Frigate", "Submarine"};
                    QStringList allCategories = {"Air", "Ground", "Marine"};

                    if (allCategories.contains(categoryFilter)) {
                        // ← clicked "Air" / "Ground" / "Marine" header
                        // map category name → categoryValue stored on entity
                        QMap<QString, QString> catToEntityCat = {
                            {"Air",    "Air"},
                            {"Ground", "Ground"},
                            {"Marine", "Marine"}
                        };
                        QString expected = catToEntityCat.value(categoryFilter, categoryFilter);
                        if (categoryValue != expected) continue;          // show all subcats of this category
                    } else if (allSubCats.contains(categoryFilter)) {
                        // ← clicked a specific subcat like "Aircraft", "Tank"
                        if (subCatValue != categoryFilter) continue;
                    } else {
                        // ← fallback: match category
                        if (categoryValue != categoryFilter) continue;
                    }
                }

                displayName = eName + " (" + (subCatValue.isEmpty() ? categoryValue : subCatValue) + ")";
            }
            else if (profileName == "Sensor") {
                QString subType = "";
                auto it = m_hierarchy->Sensors.find(entityId);
                if (it != m_hierarchy->Sensors.end() && it->second) {
                    subType = it->second->subTypeToString(it->second->subType);
                }
                if (!categoryFilter.isEmpty() && categoryFilter != "All") {
                    if (subType != categoryFilter) continue;
                }
                displayName = subType.isEmpty() ? eName : eName + " (" + subType + ")";
            } else {
                displayName = eName;
            }

            if (!entityMap.contains(displayName)) {
                entityNames.append(displayName);
                entityMap.insert(displayName, QVariantList{QString::fromStdString(entityId), profileName});
            }
        }

        for (const auto& [folderId, folder] : profile->Folders)
        {
            if (!folder) continue;
            for (const auto& [entityId, entity] : folder->Entities)
            {
                if (!entity) continue;
                QString eName = QString::fromStdString(entity->Name).trimmed();
                if (eName.isEmpty()) continue;
                QString displayName;
                // ✅ FIXED CODE:
                if (profileName == "Platform") {
                    QString categoryValue = QString::fromStdString(
                        entity->CategoryNames[static_cast<int>(entity->category)]
                        );

                    QJsonObject entityJson = entity->toJson();
                    QString subCatValue = "";
                    if (entityJson.contains("SubCategory") &&
                        entityJson["SubCategory"].isObject()) {
                        subCatValue = entityJson["SubCategory"].toObject()["value"].toString();
                    }

                    if (!categoryFilter.isEmpty() && categoryFilter != "All") {
                        QStringList allSubCats = {"Aircraft", "Helicopter", "UAV",
                                                  "Tank", "GroundRadar", "Human",
                                                  "Ship", "Frigate", "Submarine"};
                        if (allSubCats.contains(categoryFilter)) {
                            if (subCatValue != categoryFilter) continue; // ✅ subCat filter
                        } else {
                            if (categoryValue != categoryFilter) continue;
                        }
                    }

                    displayName = eName + " (" + (subCatValue.isEmpty() ? categoryValue : subCatValue) + ")";
                } else if (profileName == "Sensor") {
                    QString subType = "";
                    auto it = m_hierarchy->Sensors.find(entityId);
                    if (it != m_hierarchy->Sensors.end() && it->second) {
                        subType = it->second->subTypeToString(it->second->subType);
                    }
                    if (!categoryFilter.isEmpty() && categoryFilter != "All") {
                        if (subType != categoryFilter) continue;
                    }
                    displayName = subType.isEmpty() ? eName : eName + " (" + subType + ")";
                } else {
                    displayName = eName;
                }
                if (!entityMap.contains(displayName)) {
                    entityNames.append(displayName);
                    entityMap.insert(displayName, QVariantList{QString::fromStdString(entityId), profileName});
                }
            }
        }
    }
    entityNames.sort(Qt::CaseInsensitive);
    if (entityCompleter) {
        QStringListModel *model = qobject_cast<QStringListModel*>(entityCompleter->model());
        if (model) {
            model->setStringList(entityNames);
        }
    }
}

void AddItemDialog::setupUI(DialogType type)
{
    if (this->window()) {
    }
    QWidget* currentWidget = this->parentWidget();
    int level = 0;
    while (currentWidget) {
        if (currentWidget->isWindow()) {
        }
        currentWidget = currentWidget->parentWidget();
        level++;
    }
    QWidget *mainWidget = new QWidget(this);
    mainWidget->setStyleSheet("background-color: #0F2636;");
    QVBoxLayout *mainLayout = new QVBoxLayout(mainWidget);
    mainLayout->setSpacing(10);
    mainLayout->setContentsMargins(15, 15, 15, 15);
    QString itemType = (type == EntityType) ? "Entity" : "Folder";
    bool isDatabaseEditor = false;
    if (!m_editorContext.isEmpty()) {
        isDatabaseEditor = (m_editorContext == "database");
    } else {
        isDatabaseEditor = detectDatabaseEditorFromWindow();
    }
    // Determine dialog mode and type
    bool isProfileSensorAdd = (specificType == "Sensor");
    bool isComponentSensorAdd = (m_dialogMode == ComponentSensorMode) || (specificType == "sensors");
    bool isComponentIFFAdd = (m_dialogMode == ComponentIFFMode) || (specificType == "iffs");
    bool isComponentRadioAdd = (m_dialogMode == ComponentRadioMode) || (specificType == "radios");
    bool isComponentWeaponAdd = (m_dialogMode == ComponentWeaponMode) || (specificType == "weapons");
    bool isForComponentAdd = isComponentSensorAdd || isComponentIFFAdd || isComponentRadioAdd || isComponentWeaponAdd;
    bool isForSensor = isProfileSensorAdd || isComponentSensorAdd;
    bool shouldShowEntitySelection = false;
    if (!isDatabaseEditor && type == EntityType && !isForComponentAdd &&
        (specificType.isEmpty() || specificType == "Platform" ||
         specificType == "SpecialZone" || specificType == "FixedPoints" ||
         specificType == "Radio" || specificType == "Sensor" ||
         specificType == "Weapon" || specificType == "IFF" ||
         specificType == "Formation")) {
        shouldShowEntitySelection = true;
    }
    if (!isDatabaseEditor && isForComponentAdd) {
        shouldShowEntitySelection = true;
    }
    if (shouldShowEntitySelection && m_hierarchy) {
        if (!isDatabaseEditor && isForSensor) {
            QHBoxLayout *addNewBtnLayout = new QHBoxLayout();
            addNewBtnLayout->addStretch();
            m_addNewBtn = new QPushButton("✚  Add New", this);
            m_addNewBtn->setCheckable(true);
            m_addNewBtn->setChecked(false);
            m_addNewBtn->setFixedHeight(28);
            m_addNewBtn->setCursor(Qt::PointingHandCursor);
            m_addNewBtn->setStyleSheet(
                "QPushButton {"
                "  background-color: #1565C0;"
                "  color: white;"
                "  border: none;"
                "  border-radius: 4px;"
                "  padding: 4px 14px;"
                "  font-size: 12px;"
                "  font-weight: bold;"
                "}"
                "QPushButton:hover  { background-color: #1976D2; }"
                "QPushButton:checked {"
                "  background-color: #424242;"
                "  color: #90CAF9;"
                "}"
                "QPushButton:checked:hover { background-color: #616161; }"
                );
            addNewBtnLayout->addWidget(m_addNewBtn);
            mainLayout->addLayout(addNewBtnLayout);
        }

        // ── Entity-search section wrapped in a container ──
        m_entitySearchContainer = new QWidget(this);
        QVBoxLayout *containerLayout = new QVBoxLayout(m_entitySearchContainer);
        containerLayout->setContentsMargins(0, 0, 0, 0);
        containerLayout->setSpacing(6);

        QHBoxLayout *entitySelectLayout = new QHBoxLayout();
        QVBoxLayout *searchLayout = new QVBoxLayout();

        QHBoxLayout *searchRowLayout = new QHBoxLayout();
        searchRowLayout->setSpacing(6);

        QLabel *searchLabel = new QLabel("Search:", this);
        searchLabel->setStyleSheet("color: white; font-weight: bold;");
        searchRowLayout->addWidget(searchLabel);

        searchRowLayout->addWidget(entitySearchLineEdit);

        entitySearchLineEdit = new QLineEdit(this);
        entitySearchLineEdit->setPlaceholderText("Type to search...");
        entitySearchLineEdit->setMinimumWidth(200);
        searchRowLayout->addWidget(entitySearchLineEdit);

        QComboBox *categoryFilterComboBox = nullptr;

        if (m_profileContext == "Platform") {
            categoryFilterComboBox = new QComboBox(this);

            QMap<QString, QStringList> catSubMap = {
                {"Air",    QStringList{"Aircraft", "Helicopter", "UAV"}},
                {"Ground", QStringList{"Tank", "GroundRadar", "Human"}},
                {"Marine", QStringList{"Ship", "Frigate", "Submarine"}}
            };

            // "All" item
            categoryFilterComboBox->addItem("  All", "All");

            QStandardItemModel* model =
                qobject_cast<QStandardItemModel*>(categoryFilterComboBox->model());

            for (auto catIt = catSubMap.begin(); catIt != catSubMap.end(); ++catIt) {
                const QString category    = catIt.key();
                const QStringList subCats = catIt.value();

                // ── Header item: store category name as data, keep it ENABLED ──
                categoryFilterComboBox->addItem("  " + category, category);  // ← data = "Air"/"Ground"/"Marine"
                int headerIndex = categoryFilterComboBox->count() - 1;
                if (model) {
                    QStandardItem* headerItem = model->item(headerIndex);
                    if (headerItem) {
                        // ← REMOVED setFlags(Qt::NoItemFlags) — keep it selectable
                        QFont f = headerItem->font();
                        f.setBold(true);
                        headerItem->setFont(f);
                        headerItem->setForeground(QColor("#64B5F6"));
                        headerItem->setBackground(QColor("#0D1F2D"));
                    }
                }

                // ── Subcategory items ─────────────────────────────────────────
                for (const QString& subCat : subCats) {
                    categoryFilterComboBox->addItem("       " + subCat, subCat);
                    int subIndex = categoryFilterComboBox->count() - 1;
                    if (model) {
                        QStandardItem* subItem = model->item(subIndex);
                        if (subItem) {
                            subItem->setForeground(QColor("#FFFFFF"));
                            subItem->setBackground(QColor("#132333"));
                        }
                    }
                }
            }

            categoryFilterComboBox->setCurrentIndex(0);
            categoryFilterComboBox->setFixedWidth(130);
            categoryFilterComboBox->setFixedHeight(28);
            categoryFilterComboBox->setStyleSheet(
                "QComboBox {"
                "  color: white;"
                "  background-color: #1A3652;"
                "  border: 1px solid #27446d;"
                "  border-radius: 4px;"
                "  padding: 2px 6px;"
                "  font-size: 12px;"
                "}"
                "QComboBox::drop-down {"
                "  border: none;"
                "  width: 20px;"
                "}"
                "QComboBox::down-arrow {"
                "  image: url(:/icons/images/down.png);"
                "  width: 12px; height: 12px;"
                "}"
                "QComboBox QAbstractItemView {"
                "  background-color: #132333;"
                "  color: white;"
                "  border: 1px solid #27446d;"
                "  border-radius: 0px;"
                "  selection-background-color: #1565C0;"
                "  selection-color: white;"
                "  padding: 2px 0px;"
                "  outline: none;"
                "}"
                "QComboBox QAbstractItemView::item {"
                "  min-height: 22px;"
                "  padding: 2px 4px;"
                "}"
                "QComboBox QAbstractItemView::item:selected {"
                "  background-color: #1565C0;"
                "}");
            searchRowLayout->addWidget(categoryFilterComboBox);
        }

        else if (m_profileContext == "Sensor") {
            categoryFilterComboBox = new QComboBox(this);
            QStringList filterTypes;
            filterTypes.append("All");
            filterTypes += m_hierarchy
                               ? m_hierarchy->getAvailableSensorTypes()
                               : QStringList({"Generic", "CSM", "ESM", "EO", "IR", "Sonar", "AIS", "ADSB", "AESA"});
            categoryFilterComboBox->addItems(filterTypes);            categoryFilterComboBox->setCurrentText("All");
            categoryFilterComboBox->setFixedWidth(70);
            categoryFilterComboBox->setStyleSheet(
                "QComboBox { color: white; background-color: #1A3652; "
                "border: 1px solid #27446d; padding: 4px; }"
                "QComboBox::drop-down { border: none; width: 20px; }"
                "QComboBox::down-arrow { image: url(:/icons/images/down.png); "
                "width: 16px; height: 16px; }"
                "QComboBox QAbstractItemView { background-color: #1A3652; "
                "color: white; selection-background-color: #27446d; }");
            searchRowLayout->addWidget(categoryFilterComboBox);
        }
        searchLayout->addLayout(searchRowLayout);
        entitySelectLayout->addLayout(searchLayout);
        containerLayout->addLayout(entitySelectLayout);

        entityCompleter = new QCompleter(this);
        entityCompleter->setCaseSensitivity(Qt::CaseInsensitive);
        entityCompleter->setFilterMode(Qt::MatchContains);
        entityCompleter->setCompletionMode(QCompleter::PopupCompletion);
        entityCompleter->setMaxVisibleItems(15);

        if (entityCompleter->popup()) {
            entityCompleter->popup()->setStyleSheet(AddItemDialogStyles::CompleterPopup);
        }

        QAction *showAllAction = new QAction(this);
        showAllAction->setIcon(QApplication::style()->standardIcon(QStyle::SP_ArrowDown));
        entitySearchLineEdit->addAction(showAllAction, QLineEdit::TrailingPosition);

        QStringListModel *completerModel = new QStringListModel(this);
        entityCompleter->setModel(completerModel);
        entitySearchLineEdit->setCompleter(entityCompleter);

        populateEntityProfiles(m_profileContext, "");
        if (categoryFilterComboBox) {
            connect(categoryFilterComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged),
                    this, [=](int index) {
                        QString selectedData = categoryFilterComboBox->itemData(index).toString();

                        // "All" → no filter
                        // "Air"/"Ground"/"Marine" → filter by category (shows all subcats of that category)
                        // "Aircraft"/"Tank"/etc. → filter by exact subcat
                        QString filterToApply = (selectedData == "All" || selectedData.isEmpty())
                                                    ? "" : selectedData;

                        populateEntityProfiles(m_profileContext, filterToApply);
                        entitySearchLineEdit->clear();
                        selectedEntityId.clear();

                        if (entityCompleter) {
                            QStringListModel* mdl =
                                qobject_cast<QStringListModel*>(entityCompleter->model());
                            if (mdl && !mdl->stringList().isEmpty()) {
                                entitySearchLineEdit->setFocus();
                                QTimer::singleShot(100, this, [=]() {
                                    entityCompleter->setCompletionPrefix("");
                                    entityCompleter->complete();
                                });
                            }
                        }
                    });
        }    // connect(categoryFilterComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged),


        connect(showAllAction, &QAction::triggered, this, [=]() {
            if (entityCompleter) {
                QString currentCategory = categoryFilterComboBox ?
                                              categoryFilterComboBox->currentData().toString() : "All";
                QString filterToApply = (currentCategory == "All" || currentCategory.isEmpty())
                                            ? "" : currentCategory;
                populateEntityProfiles(m_profileContext, filterToApply);
                entitySearchLineEdit->setFocus();
                QTimer::singleShot(100, this, [=]() {
                    entityCompleter->setCompletionPrefix("");
                    entityCompleter->complete();
                });
            }
        });
        connect(entityCompleter, QOverload<const QString &>::of(&QCompleter::activated),
                this, [=](const QString &text) {
                    if (entityMap.contains(text)) {
                        QVariantList entityData = entityMap[text];
                        if (entityData.size() >= 2) {
                            selectedEntityId = entityData[0].toString();
                            QString profileName = entityData[1].toString();
                            QString entityName = text;
                            int parenIndex = entityName.indexOf(" (");
                            if (parenIndex != -1) entityName = entityName.left(parenIndex);
                            if (nameLineEdit) nameLineEdit->setText(entityName);
                            entitySearchLineEdit->setText(text);
                            entitySearchLineEdit->setCursorPosition(0);
                            populateComponentsFromEntity(selectedEntityId, profileName);

                            if (isForSensor && sensorTypeComboBox) {
                                Hierarchy* lib = m_hierarchy;
                                if (lib) {
                                    auto it = lib->Sensors.find(selectedEntityId.toStdString());
                                    if (it != lib->Sensors.end() && it->second) {
                                        Sensor* sens = it->second;
                                        QString subTypeStr = sens->subTypeToString(sens->subType);
                                        int idx = sensorTypeComboBox->findText(subTypeStr);
                                        if (idx >= 0) sensorTypeComboBox->setCurrentIndex(idx);
                                    }
                                }
                                if (!isDatabaseEditor) {
                                    sensorTypeComboBox->setEnabled(false);
                                    sensorTypeComboBox->setToolTip("Type automatically set from selected sensor");
                                }
                            }
                        }
                    }
                });

        auto showCompleterPopup = [=]() {
            if (entityCompleter && entitySearchLineEdit->text().isEmpty()) {
                entitySearchLineEdit->clear();
                selectedEntityId.clear();
                QString currentFilter = profileFilterComboBox ?
                                            profileFilterComboBox->currentText() : "";
                QString currentCategory = "All";
                if (categoryFilterComboBox) currentCategory = categoryFilterComboBox->currentText();
                if (currentFilter == "All Profiles") {
                    populateEntityProfiles("", currentCategory);
                } else {
                    populateEntityProfiles(currentFilter, currentCategory);
                }
                entityCompleter->complete();
            }
        };

        connect(entitySearchLineEdit, &QLineEdit::cursorPositionChanged, this, [=](int oldPos, int newPos) {
            Q_UNUSED(oldPos); Q_UNUSED(newPos);
            if (!entitySearchLineEdit->hasFocus()) showCompleterPopup();
        });

        connect(entitySearchLineEdit, &QLineEdit::textChanged, this, [=](const QString& text) {
            if (text.isEmpty()) {
                selectedEntityId.clear();
                if (sensorTypeComboBox) {
                    sensorTypeComboBox->setEnabled(true);
                    sensorTypeComboBox->setToolTip("");
                    if (sensorTypeComboBox->findText("Generic") >= 0)
                        sensorTypeComboBox->setCurrentText("Generic");
                    else if (sensorTypeComboBox->count() > 0)
                        sensorTypeComboBox->setCurrentIndex(0);                }
            }
        });

        mainLayout->addWidget(m_entitySearchContainer);
        if (m_addNewBtn) {
            connect(m_addNewBtn, &QPushButton::toggled, this, [=](bool checked) {
                m_addNewMode = checked;
                m_entitySearchContainer->setVisible(!checked);
                if (m_sensorTypeContainer) {
                    m_sensorTypeContainer->setVisible(isDatabaseEditor ? true : checked);
                }
                m_addNewBtn->setText(checked ? "← Search DB" : "✚  Add New");
                if (checked) {
                    selectedEntityId.clear();
                    if (entitySearchLineEdit) {
                        entitySearchLineEdit->clear();
                        entitySearchLineEdit->setCompleter(nullptr);
                        if (entityCompleter && entityCompleter->popup())
                            entityCompleter->popup()->hide();
                    }
                    if (nameLineEdit)     nameLineEdit->setText("Sensor");
                    if (sensorTypeComboBox) {
                        sensorTypeComboBox->setEnabled(true);
                        sensorTypeComboBox->setToolTip("");
                        sensorTypeComboBox->setCurrentText("Generic");
                    }
                } else {
                    if (entitySearchLineEdit) {
                        entitySearchLineEdit->clear();
                        entitySearchLineEdit->setCompleter(entityCompleter);
                    }
                    selectedEntityId.clear();
                    if (nameLineEdit) nameLineEdit->setText("Sensor");
                }
            });
        }
    }

    if (isForComponentAdd) {
        QHBoxLayout *selectProfileLayout = new QHBoxLayout();
        profileComboBox = new QComboBox(this);
        profileComboBox->setEditable(false);
        QString placeholderText;
        if (isComponentSensorAdd) placeholderText = "Select Sensor";
        else if (isComponentIFFAdd) placeholderText = "Select IFF";
        else if (isComponentRadioAdd) placeholderText = "Select Radio";
        else if (isComponentWeaponAdd) placeholderText = "Select Weapon";
        profileComboBox->addItem(placeholderText);
        profileComboBox->setCurrentIndex(0);
        profileComboBox->setItemData(0, QColor(Qt::gray), Qt::TextColorRole);
        if (isComponentSensorAdd) {
            populateSensorProfiles();
        } else if (isComponentIFFAdd) {
            populateIFFProfiles();
        } else if (isComponentRadioAdd) {
            populateRadioProfiles();
        } else if (isComponentWeaponAdd) {
            populateWeaponProfiles();
        }
        profileComboBox->setMinimumWidth(250);
        selectProfileLayout->addWidget(profileComboBox);
        selectProfileLayout->addStretch();
        mainLayout->addLayout(selectProfileLayout);
        if (isForComponentAdd) {
            profileComboBox->setVisible(false);
        }
    }

    QHBoxLayout *nameLayout = new QHBoxLayout();
    QString nameLabelText;
    QString lowerSpecificType = specificType.toLower();
    if (specificType.isEmpty() || specificType == "Platform" || specificType == "Entity") {
        nameLabelText = "Entity Name:";
    } else {
        QMap<QString, QString> typeDisplayMap;
        typeDisplayMap["sensor"] = "Sensor";
        typeDisplayMap["weapon"] = "Weapon";
        typeDisplayMap["formation"] = "Formation";
        typeDisplayMap["radio"] = "Radio";
        typeDisplayMap["iff"] = "IFF";
        typeDisplayMap["specialzone"] = "Special Zone";
        typeDisplayMap["fixedpoints"] = "Fixed Point";
        typeDisplayMap["fixedpoint"] = "Fixed Point";
        if (typeDisplayMap.contains(lowerSpecificType)) {
            nameLabelText = typeDisplayMap[lowerSpecificType] + " Name:";
        } else {
            nameLabelText = "Name:";
        }
    }
    QLabel *nameLabel = new QLabel(nameLabelText, this);
    nameLabel->setStyleSheet("color: white; font-weight: bold;");
    nameLayout->addWidget(nameLabel);
    nameLineEdit = new QLineEdit(this);
    QString defaultName;
    QMap<QString, QString> defaultNameMap;
    defaultNameMap["sensor"] = "Sensor";
    defaultNameMap["weapon"] = "Weapon";
    defaultNameMap["formation"] = "Formation";
    defaultNameMap["radio"] = "Radio";
    defaultNameMap["iff"] = "IFF";
    defaultNameMap["specialzone"] = "SpecialZone";
    defaultNameMap["fixedpoints"] = "FixedPoint";
    defaultNameMap["fixedpoint"] = "FixedPoint";
    if (defaultNameMap.contains(lowerSpecificType)) {
        defaultName = defaultNameMap[lowerSpecificType];
    } else if (isForSensor) {
        defaultName = "Sensor";
    } else if (isComponentIFFAdd) {
        defaultName = "IFF";
    } else if (isComponentRadioAdd) {
        defaultName = "Radio";
    } else if (isComponentWeaponAdd) {
        defaultName = "Weapon";
    } else {
        defaultName = getDefaultName(type);
    }
    nameLineEdit->setText(defaultName);
    QString placeholderText;
    if (specificType.isEmpty() || specificType == "Platform" || specificType == "Entity") {
        placeholderText = "Enter Entity name";
    } else {
        QString typeName = defaultNameMap.contains(lowerSpecificType) ?
                               defaultNameMap[lowerSpecificType] : "Entity";
        placeholderText = "Enter " + typeName + " name";
    }
    nameLineEdit->setPlaceholderText(placeholderText);
    nameLineEdit->setStyleSheet("color: white; background-color: #1A3652; border: 1px solid #27446d;");
    nameLayout->addWidget(nameLineEdit);
    mainLayout->addLayout(nameLayout);
    if (isForSensor) {
        m_sensorTypeContainer = new QWidget(this);
        QHBoxLayout *sensorTypeLayout = new QHBoxLayout(m_sensorTypeContainer);
        sensorTypeLayout->setContentsMargins(0, 0, 0, 0);
        QLabel *sensorLabel = new QLabel("Sensor Type:", this);
        sensorLabel->setStyleSheet("color: white;");
        sensorTypeLayout->addWidget(sensorLabel);
        sensorTypeComboBox = new QComboBox(this);
        QStringList sensorTypes = m_hierarchy
                                      ? m_hierarchy->getAvailableSensorTypes()
                                      : QStringList({"Generic", "CSM", "ESM", "EO", "IR", "Sonar", "AIS", "ADSB", "AESA"});
        sensorTypeComboBox->addItems(sensorTypes);        sensorTypeComboBox->setCurrentText("Generic");
        sensorTypeComboBox->setStyleSheet("color: white;");
        sensorTypeLayout->addWidget(sensorTypeComboBox);
        sensorTypeLayout->addStretch();
        if (isDatabaseEditor) {
            m_sensorTypeContainer->setVisible(true);
        } else {
            m_sensorTypeContainer->setVisible(false);
        }
        mainLayout->addWidget(m_sensorTypeContainer);
    }
    bool shouldShowNumberField = false;
    QStringList showNumberForTypes = {"Platform", "SpecialZone", "FixedPoints", "Entity", ""};
    QStringList hideNumberForTypes = {"Sensor", "Weapon", "Formation", "Radio", "IFF", "FixedPoint"};
    if (showNumberForTypes.contains(specificType) ||
        (specificType.isEmpty() && type == EntityType)) {
        shouldShowNumberField = true;
    }
    if (hideNumberForTypes.contains(lowerSpecificType)) {
        shouldShowNumberField = false;
    }
    if (type == EntityType && !isForComponentAdd && !isForSensor && shouldShowNumberField) {
        bool showCountField = true;

        if (!isDatabaseEditor) {
            showCountField = !shouldShowEntitySelection || !entitySearchLineEdit || entitySearchLineEdit->text().isEmpty();
        }
        if (showCountField) {
            QHBoxLayout *numberLayout = new QHBoxLayout();
            QString countLabel;
            QString placeholderText;
            // Map specific types to their count labels
            QMap<QString, QString> countLabelMap;
            countLabelMap["platform"] = "Entity Count:";
            countLabelMap["entity"] = "Entity Count:";
            countLabelMap["specialzone"] = "SpecialZone Count:";
            countLabelMap["special zone"] = "SpecialZone Count:";
            countLabelMap["fixedpoints"] = "Point Count:";
            countLabelMap["fixedpoint"] = "Point Count:";
            countLabelMap["sensor"] = "Sensor Count:";
            countLabelMap["weapon"] = "Weapon Count:";
            countLabelMap["formation"] = "Formation Count:";
            countLabelMap["radio"] = "Radio Count:";
            countLabelMap["iff"] = "IFF Count:";
            if (countLabelMap.contains(lowerSpecificType)) {
                countLabel = countLabelMap[lowerSpecificType];
            } else if (specificType.isEmpty()) {
                countLabel = "Entity Count:";
            } else {
                QString displayType = specificType;
                if (!displayType.isEmpty()) {
                    displayType[0] = displayType[0].toUpper();
                }
                countLabel = displayType + " Count:";
            }
            if (countLabel.contains("Platform")) {
                placeholderText = "Enter number of Entity (default: 1)";
            } else if (countLabel.contains("Sensor")) {
                placeholderText = "Enter number of sensors (default: 1)";
            } else if (countLabel.contains("Weapon")) {
                placeholderText = "Enter number of weapons (default: 1)";
            } else if (countLabel.contains("Formation")) {
                placeholderText = "Enter number of formations (default: 1)";
            } else if (countLabel.contains("Radio")) {
                placeholderText = "Enter number of radios (default: 1)";
            } else if (countLabel.contains("IFF")) {
                placeholderText = "Enter number of IFFs (default: 1)";
            } else if (countLabel.contains("Zone")) {
                placeholderText = "Enter number of SpecialZone (default: 1)";
            } else if (countLabel.contains("Point")) {
                placeholderText = "Enter number of points (default: 1)";
            } else if (countLabel.contains("Entity")) {
                placeholderText = "Enter number of entities (default: 1)";
            } else {
                placeholderText = "Enter count (default: 1)";
            }
            QLabel *countLabelWidget = new QLabel(countLabel, this);
            countLabelWidget->setStyleSheet("color: white;");
            numberLayout->addWidget(countLabelWidget);
            numberLineEdit = new QLineEdit(this);
            numberLineEdit->setText("1");
            numberLineEdit->setPlaceholderText(placeholderText);
            numberLineEdit->setValidator(new QIntValidator(1, 10000, this));
            numberLayout->addWidget(numberLineEdit);
            mainLayout->addLayout(numberLayout);
            if (shouldShowNumberField && !isForComponentAdd && !isForSensor) {
                QHBoxLayout *teamLayout = new QHBoxLayout();
                QLabel *teamLabel = new QLabel("Team:", this);
                teamLabel->setStyleSheet("color: white;");
                teamLayout->addWidget(teamLabel);
                teamSelectComboBox = new QComboBox(this);
                QStringList teams = {"None", "RedTeam", "BlueTeam", "GreenTeam",
                                     "YellowTeam", "GreyTeam", "AlphaTeam",
                                     "BetaTeam", "GammaTeam"};
                teamSelectComboBox->addItems(teams);
                teamSelectComboBox->setCurrentText("None");
                teamSelectComboBox->setStyleSheet(
                    "QComboBox { color: white; background-color: #1A3652; border: 1px solid #27446d; padding: 4px 8px; }"
                    "QComboBox::drop-down { border: none; background: transparent; width: 20px; }"
                    "QComboBox::down-arrow { image: url(:/icons/images/down.png); width: 12px; height: 12px; border: none; }"
                    "QComboBox::down-arrow:on { image: url(:/icons/images/down.png); }"
                    "QComboBox QAbstractItemView { background-color: #1A3652; color: white; selection-background-color: #27446d; }");
                teamLayout->addWidget(teamSelectComboBox);
                teamLayout->addStretch();
                mainLayout->addLayout(teamLayout);
            }
        }
    }

    bool isPlatformProfile = (type == EntityType && !isForComponentAdd && !isForSensor &&
                              (specificType.isEmpty() || specificType == "Platform"));
    if (isPlatformProfile && !isDatabaseEditor) {
        setupScSection();
        mainLayout->addWidget(scCheckBox);
        mainLayout->addWidget(scOptionsGroup);
    }
    bool shouldCreateComponents = true;
    bool shouldShowComponents = false;
    bool createComponents = (type == EntityType && !isForComponentAdd && !isForSensor &&
                             (specificType.isEmpty() || specificType == "Platform" ||
                              specificType == "SpecialZone" || specificType == "FixedPoints" ||
                              specificType == "Entity"));
    if (createComponents) {
        QGroupBox *componentsGroup = new QGroupBox("Components", this);
        componentsGroup->setStyleSheet(AddItemDialogStyles::GroupBox);
        componentsGroup->setVisible(false);
        QVBoxLayout *componentsLayout = new QVBoxLayout();
        Entity *entity = nullptr;
        QString effectiveType = specificType.isEmpty() ? "Entity" : specificType;
        if (effectiveType == "Platform") {
            entity = new Platform(nullptr);
        } else if (effectiveType == "FixedPoints") {
            entity = new FixedPoints(nullptr);
        } else if (effectiveType == "SpecialZone") {
            entity = new Specialzone(nullptr);
        } else {
            entity = new Platform(nullptr);
        }
        if (entity) {
            std::vector<std::string> supportedComponents = entity->getSupportedComponents();
            QMap<QString, Component*> uniqueComponents;

            for (const std::string &component : supportedComponents) {
                uniqueComponents.insert(demangleComponentName(component), nullptr);
            }
            for (auto it = uniqueComponents.begin(); it != uniqueComponents.end(); ++it) {
                QString displayName = it.key();
                QString camelCaseName = toCamelCase(displayName);
                QCheckBox *checkBox = new QCheckBox(displayName, this);
                if (camelCaseName == "transform") {
                    checkBox->setChecked(true);
                    checkBox->setEnabled(false);
                } else {
                    checkBox->setChecked(true);
                }
                componentCheckboxes.insert(camelCaseName, checkBox);
                componentsLayout->addWidget(checkBox);
            }
            componentsGroup->setLayout(componentsLayout);
            delete entity;
        }
    }
    mainLayout->addStretch();
    // Dialog buttons
    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok |
                                                           QDialogButtonBox::Cancel, this);
    buttonBox->setStyleSheet(AddItemDialogStyles::ButtonBox);
    connect(buttonBox, &QDialogButtonBox::accepted, this, [this]() {
        if (validateInputs()) {
            accept();
        }
    });
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    mainLayout->addWidget(buttonBox);
    bool isSimpleDialog = (specificType == "Formation" ||
                           specificType == "IFF" ||
                           specificType == "Radio" ||
                           specificType == "Sensor" ||
                           (specificType == "Weapon" && isDatabaseEditor) ||
                           specificType == "FixedPoint" ||
                           isProfileSensorAdd ||
                           isComponentSensorAdd ||
                           isComponentIFFAdd ||
                           isComponentRadioAdd);
    if (isSimpleDialog) {
        QVBoxLayout *dialogLayout = new QVBoxLayout(this);
        dialogLayout->addWidget(mainWidget);
        setLayout(dialogLayout);
        mainWidget->adjustSize();
        adjustSize();
        QString windowTitle;
        QMap<QString, QString> titleMap;
        titleMap["sensor"] = "Add Sensor";
        titleMap["weapon"] = "Add Weapon";
        titleMap["formation"] = "Add Formation";
        titleMap["radio"] = "Add Radio";
        titleMap["iff"] = "Add IFF";
        titleMap["specialzone"] = "Add Special Zone";
        titleMap["fixedpoints"] = "Add Fixed Point";
        titleMap["fixedpoint"] = "Add Fixed Point";
        titleMap["platform"] = "Add Entity";
        titleMap["entity"] = "Add Entity";
        if (titleMap.contains(lowerSpecificType)) {
            windowTitle = titleMap[lowerSpecificType];
        } else if (isComponentSensorAdd) {
            windowTitle = "Add Sensors";
        } else if (isComponentIFFAdd) {
            windowTitle = "Add IFFs";
        } else if (isComponentRadioAdd) {
            windowTitle = "Add Radios";
        } else if (isComponentWeaponAdd) {
            windowTitle = "Add Weapons";
        } else if (isProfileSensorAdd) {
            windowTitle = "Add Sensor";
        } else {
            windowTitle = "Add Entity";
        }
        if (isComponentSensorAdd || isComponentIFFAdd || isComponentRadioAdd || isComponentWeaponAdd) {
            setMinimumSize(420, 250); resize(350, 300);
        } else if (isProfileSensorAdd) {
            setMinimumSize(350, 200); resize(350, 260);
        } else if (specificType == "Sensor") {
            setMinimumSize(420, 180); resize(320, 220);
        } else if (specificType == "Weapon" || specificType == "Formation" ||
                   specificType == "IFF" || specificType == "Radio" ||
                   specificType == "FixedPoint") {
            setMinimumSize(350, 180); resize(350, 220);
        } else {
            setMinimumSize(350, 180); resize(350, 220);
        }
        setWindowTitle(windowTitle);
    } else {
        QScrollArea *scrollArea = new QScrollArea(this);
        scrollArea->setWidgetResizable(true);
        scrollArea->setWidget(mainWidget);
        scrollArea->setFrameShape(QFrame::NoFrame);
        scrollArea->setStyleSheet(AddItemDialogStyles::ScrollArea);
        QVBoxLayout *dialogLayout = new QVBoxLayout(this);
        dialogLayout->addWidget(scrollArea);
        setLayout(dialogLayout);
        QString windowTitle;
        if (specificType == "Platform" || specificType.isEmpty()) {
            windowTitle = "Add Entity";
            if (isDatabaseEditor) {
                setMinimumSize(500, 400); resize(500, 550);
            } else {
                setMinimumSize(500, 400); resize(500, 700);
            }
        } else if (specificType == "SpecialZone") {
            windowTitle = "Add Special Zone";
            setMinimumSize(500, 400); resize(500, 550);
        } else if (specificType == "FixedPoints") {
            windowTitle = "Add Fixed Point";
            setMinimumSize(500, 400); resize(500, 550);
        } else if (specificType == "Weapon") {
            windowTitle = "Add Weapon";
            setMinimumSize(450, 280); resize(450, 320);
        } else {
            windowTitle = "Add Entity";
            setMinimumSize(450, 350); resize(450, 450);
        }
        setWindowTitle(windowTitle);
        scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
        scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    }
    // Focus management
    if (nameLineEdit) {
        nameLineEdit->setFocus();
        nameLineEdit->selectAll();
    }
}

// %%% Accessor Methods %%%

/* Get entered item name */
QString AddItemDialog::getName() const {
    return nameLineEdit ? nameLineEdit->text().trimmed() : "";
}

/* Get number of entities */
int AddItemDialog::getNumber() const {
    if (numberLineEdit && !numberLineEdit->text().trimmed().isEmpty()) {
        return numberLineEdit->text().trimmed().toInt();
    }
    return 1;
}
void AddItemDialog::setNumber(int value){
    if(numberLineEdit){
        numberLineEdit->setText(QString::number(value));
    }
}
void AddItemDialog::populateComponentsFromEntity(const QString& entityId, const QString& profileName)
{
    Q_UNUSED(profileName);
    selectedEntityId = entityId;
    entityComponents.clear();
    for (auto it = componentCheckboxes.begin(); it != componentCheckboxes.end(); ++it) {
        QString compName = it.key();
        if (compName == "transform" && it.value()->isEnabled()) {
            it.value()->setChecked(true);
        } else if (compName != "transform") {
            it.value()->setChecked(true);
        }
        entityComponents[compName] = true;
    }
    entityComponents["_FromDB"] = true;
    entityComponents["_sourceEntityId"] = entityId;
}

QVariantMap AddItemDialog::getEntityComponents() const
{
    // If we selected from library, return the stored components
    if (!selectedEntityId.isEmpty() && !entityComponents.isEmpty()) {
        return entityComponents;
    }
    QVariantMap components;
    for (auto it = componentCheckboxes.begin(); it != componentCheckboxes.end(); ++it) {
        components.insert(it.key(), it.value()->isChecked());
    }
    return components;
}
// Get selected entity ID
QString AddItemDialog::getSelectedEntityId() const
{
    return selectedEntityId;
}
/* Get selected components map */
QVariantMap AddItemDialog::getComponents() const {
    QVariantMap components;
    for (auto it = componentCheckboxes.begin(); it != componentCheckboxes.end(); ++it) {
        components.insert(it.key(), it.value()->isChecked());
    }
    return components;
}
/* Get selected sensor type */
QString AddItemDialog::getSensorType() const {
    return sensorTypeComboBox ? sensorTypeComboBox->currentText() : "Generic";
}
/* Get selected profile ID */
QString AddItemDialog::getProfileId() const {
    // If the user selected an entity from the search, use its ID as the profile ID
    if (!selectedEntityId.isEmpty()) {
        return selectedEntityId;
    }
    // Otherwise, if the profile combo box is visible and has a selection, use that
    if (profileComboBox && profileComboBox->currentIndex() > 0) {
        return profileComboBox->currentData().toString();
    }
    return "";
}
/* Get selected profile name */
QString AddItemDialog::getProfileName() const {
    if (profileComboBox && profileComboBox->currentIndex() > 0) {
        return profileComboBox->currentText();
    }
    return "";
}

/* Get component type based on dialog mode */
QString AddItemDialog::getComponentType() const {
    if (m_dialogMode == ComponentSensorMode) return "sensors";
    if (m_dialogMode == ComponentIFFMode) return "iffs";
    if (m_dialogMode == ComponentRadioMode) return "radios";
    if (m_dialogMode == ComponentWeaponMode) return "weapons";
    return "";
}

// %%% Profile Population Methods %%%

/* Generic method to populate profiles from hierarchy */
void AddItemDialog::populateProfiles(const QString& profileType)
{
    if (!profileComboBox || !m_hierarchy) {
        return;
    }
    // Find profile category
    for (const auto& [profileId, profile] : m_hierarchy->ProfileCategories) {
        if (!profile) continue;
        QString profileName = QString::fromStdString(profile->Name);
        if (profileName == profileType) {
            // Collect and sort entities by name
            QMap<QString, QString> profileEntities;
            for (const auto& [entityId, entity] : profile->Entities) {
                if (!entity) continue;
                QString entityIdStr = QString::fromStdString(entityId);
                QString entityName = QString::fromStdString(entity->Name);
                if (!entityName.isEmpty()) {
                    profileEntities.insert(entityName, entityIdStr);
                }
            }
            // Add sorted entities to combo box
            for (auto it = profileEntities.begin(); it != profileEntities.end(); ++it) {
                profileComboBox->addItem(it.key(), it.value());
            }
            return;
        }
    }
}

/* Populate sensor profiles */
void AddItemDialog::populateSensorProfiles()
{
    populateProfiles("Sensor");
}

/* Populate IFF profiles */
void AddItemDialog::populateIFFProfiles()
{
    populateProfiles("IFF");
}

/* Populate radio profiles */
void AddItemDialog::populateRadioProfiles()
{
    populateProfiles("Radio");
}

/* Populate weapon profiles — mirrors Radio pattern exactly */
void AddItemDialog::populateWeaponProfiles()
{
    populateProfiles("Weapon");
}

// %%% NEW: Clear Entity Selection %%%
void AddItemDialog::clearEntitySelection()
{
    selectedEntityId.clear();
    entityComponents.clear();
    if (entitySearchLineEdit) {
        entitySearchLineEdit->clear();
    }
    if (sensorTypeComboBox) {
        sensorTypeComboBox->setEnabled(true);
        sensorTypeComboBox->setToolTip("");
        sensorTypeComboBox->setCurrentText("Generic");
    }
    if (nameLineEdit) {
        // Reset to default name based on type
        QString defaultName = "Entity";
        QMap<QString, QString> defaultNameMap;
        defaultNameMap["sensor"] = "Sensor";
        defaultNameMap["weapon"] = "Weapon";
        defaultNameMap["formation"] = "Formation";
        defaultNameMap["radio"] = "Radio";
        defaultNameMap["iff"] = "IFF";
        defaultNameMap["specialzone"] = "SpecialZone";
        defaultNameMap["fixedpoints"] = "FixedPoint";
        defaultNameMap["fixedpoint"] = "FixedPoint";
        QString lowerType = specificType.toLower();
        if (defaultNameMap.contains(lowerType)) {
            defaultName = defaultNameMap[lowerType];
        }
        nameLineEdit->setText(defaultName);
    }
    for (auto it = componentCheckboxes.begin(); it != componentCheckboxes.end(); ++it) {
        QString compName = it.key();
        if (compName == "transform") {
            it.value()->setChecked(true);
        } else {
            it.value()->setChecked(false);
        }
    }
}

// %%% Scenario Configuration Getters %%%

/* Check if scenario config is enabled */
bool AddItemDialog::isScenarioconfigEnabled() const {
    return scCheckBox ? scCheckBox->isChecked() : false;
}

/* Get scenario type */
QString AddItemDialog::getScType() const {
    return scTypeComboBox ? scTypeComboBox->currentText() : "Spread";
}

QPointF AddItemDialog::getCity() const {
    QString city = cityComboBox->currentText();
    // Check karein ki city map mein hai ya nahi
    if (indianCities.contains(city)) {
        return indianCities[city];
    }
    // Banglore Cordinates
    return QPointF(12.9716, 77.5946);
}

/* Get scenario range */
int AddItemDialog::getScRange() const {
    return rangeLineEdit ? rangeLineEdit->text().toInt() : 100;
}

/* Get minimum radio range */
int AddItemDialog::getMinRadioRange() const {
    return minRadioRangeSpinBox ? minRadioRangeSpinBox->value() : 10;
}

/* Get maximum radio range */
int AddItemDialog::getMaxRadioRange() const {
    return maxRadioRangeSpinBox ? maxRadioRangeSpinBox->value() : 100;
}

/* Get minimum radar range */
int AddItemDialog::getMinRadarRange() const {
    return minRadarRangeSpinBox ? minRadarRangeSpinBox->value() : 50;
}

/* Get maximum radar range */
int AddItemDialog::getMaxRadarRange() const {
    return maxRadarRangeSpinBox ? maxRadarRangeSpinBox->value() : 500;
}

/* Get trajectory type */
QString AddItemDialog::getTrajectory() const {
    return trajectoryComboBox ? trajectoryComboBox->currentText() : "Line";
}

/* Get minimum plane speed */
int AddItemDialog::getMinPlaneSpeed() const {
    return minPlaneSpeedSpinBox ? minPlaneSpeedSpinBox->value() : 100;
}

/* Get maximum plane speed */
int AddItemDialog::getMaxPlaneSpeed() const {
    return maxPlaneSpeedSpinBox ? maxPlaneSpeedSpinBox->value() : 500;
}

/* Get minimum turn radius */
int AddItemDialog::getMinTurnRadius() const {
    return minTurnRadiusSpinBox ? minTurnRadiusSpinBox->value() : 10;
}

/* Get maximum turn radius */
int AddItemDialog::getMaxTurnRadius() const {
    return maxTurnRadiusSpinBox ? maxTurnRadiusSpinBox->value() : 200;
}

// Helper function to detect database editor
bool AddItemDialog::detectDatabaseEditorFromWindow()
{
    QWidget* parent = this->parentWidget();
    while (parent) {
        QString windowTitle = parent->windowTitle();
        if (!windowTitle.isEmpty() && windowTitle.contains("Database Editor", Qt::CaseInsensitive)) {
            return true;
        }
        parent = parent->parentWidget();
    }
    if (this->window()) {
        QString windowTitle = this->window()->windowTitle();
        if (windowTitle.contains("Database Editor", Qt::CaseInsensitive)) {
            return true;
        }
    }
    return false;
}

bool AddItemDialog::eventFilter(QObject *watched, QEvent *event)
{
    if (watched == entityCompleter->popup() && event->type() == QEvent::Show) {
        if (entityCompleter->completionCount() == 0) {
            populateEntityProfiles(m_profileContext);
        }
    }
    return QDialog::eventFilter(watched, event);
}

// Determine profile context based on various parameters
QString AddItemDialog::determineProfileContext(const QString& specificType,
                                               DialogMode dialogMode,
                                               const QString& editorContext)
{
    QString profileContext;

    // First check dialog mode for component additions
    if (dialogMode == ComponentIFFMode) {
        return "IFF";
    }
    else if (dialogMode == ComponentRadioMode) {
        return "Radio";
    }
    else if (dialogMode == ComponentWeaponMode) {
        return "Weapon";
    }
    else if (dialogMode == ComponentSensorMode) {
        return "Sensor";
    }
    QString lowerType = specificType.toLower();
    if (lowerType == "platform" || lowerType == "entity" || lowerType.isEmpty()) {
        return "Platform";
    }
    else if (lowerType == "iff") {
        return "IFF";
    }
    else if (lowerType == "radio") {
        return "Radio";
    }
    else if (lowerType == "sensor") {
        return "Sensor";
    }
    else if (lowerType == "fixedpoints" || lowerType == "fixedpoint") {
        return "FixedPoints";
    }
    else if (lowerType == "specialzone") {
        return "SpecialZone";
    }
    else if (lowerType == "weapon") {
        return "Weapon";
    }
    else if (lowerType == "formation") {
        return "Formation";
    }
    return "";
}

// %%% Validation Methods %%%
bool AddItemDialog::validateInputs()
{
    bool isDatabaseEditor = false;
    if (!m_editorContext.isEmpty()) {
        isDatabaseEditor = (m_editorContext == "database");
    } else {
        isDatabaseEditor = detectDatabaseEditorFromWindow();
    }

    bool isComponentSensorAdd = (m_dialogMode == ComponentSensorMode) || (specificType == "sensors");
    bool isComponentIFFAdd    = (m_dialogMode == ComponentIFFMode)    || (specificType == "iffs");
    bool isComponentRadioAdd  = (m_dialogMode == ComponentRadioMode)  || (specificType == "radios");
    bool isComponentWeaponAdd = (m_dialogMode == ComponentWeaponMode) || (specificType == "weapons");
    bool isForComponentAdd    = isComponentSensorAdd || isComponentIFFAdd ||
                             isComponentRadioAdd  || isComponentWeaponAdd;

    bool isForSensor = (specificType == "Sensor") || isComponentSensorAdd;

    if (isForSensor && !isDatabaseEditor && !m_addNewMode) {
        if (!entitySearchLineEdit ||
            entitySearchLineEdit->text().trimmed().isEmpty() ||
            selectedEntityId.isEmpty())
        {
            QMessageBox msgBox(this);
            msgBox.setStyleSheet(AddItemDialogStyles::MessageBox);
            msgBox.setWindowTitle("Sensor Selection Required");
            msgBox.setText("Please select a Sensor from the Database before proceeding.\n\n"
                           "Use the search field to find and select a sensor,\n"
                           "or click '✚ Add New' to create a new one.");
            msgBox.setIcon(QMessageBox::Warning);
            msgBox.exec();
            if (entitySearchLineEdit) entitySearchLineEdit->setFocus();
            return false;
        }
    }

    bool isEntitySelectionRequired = false;
    if (!isDatabaseEditor && entitySearchLineEdit && !isForComponentAdd && !isForSensor) {
        isEntitySelectionRequired = entitySearchLineEdit->isVisible();
    }

    if (isEntitySelectionRequired && !m_addNewMode) {
        QString searchText = entitySearchLineEdit->text().trimmed();
        if (searchText.isEmpty() || selectedEntityId.isEmpty()) {
            QMessageBox msgBox(this);
            msgBox.setStyleSheet(AddItemDialogStyles::MessageBox);
            msgBox.setWindowTitle("Entity Selection Required");
            msgBox.setText("Please select an entity from the Database before proceeding.\n\n"
                           "Use the search field to find and select an entity.");
            msgBox.setIcon(QMessageBox::Warning);
            msgBox.exec();
            if (entitySearchLineEdit) entitySearchLineEdit->setFocus();
            return false;
        }
    }
    QString name = getName();
    if (name.isEmpty()) {
        QMessageBox msgBox(this);
        msgBox.setStyleSheet(AddItemDialogStyles::MessageBox);
        msgBox.setWindowTitle("Invalid Input");
        msgBox.setText("Please enter a valid name.");
        msgBox.setIcon(QMessageBox::Warning);
        msgBox.exec();
        if (nameLineEdit) nameLineEdit->setFocus();
        return false;
    }

    if (profileComboBox && profileComboBox->isVisible()) {
        if (profileComboBox->currentIndex() == 0) {
            QString componentType;
            if (m_dialogMode == ComponentSensorMode) componentType = "Sensor";
            else if (m_dialogMode == ComponentIFFMode) componentType = "IFF";
            else if (m_dialogMode == ComponentRadioMode) componentType = "Radio";
            QMessageBox msgBox(this);
            msgBox.setStyleSheet(AddItemDialogStyles::MessageBox);
            msgBox.setWindowTitle("Selection Required");
            msgBox.setText(QString("Please select a %1 from the dropdown.").arg(componentType));
            msgBox.setIcon(QMessageBox::Warning);
            msgBox.exec();
            profileComboBox->setFocus();
            return false;
        }
    }
    return true;
}
QString AddItemDialog::getSelectedTeam() const {
    if (teamSelectComboBox && teamSelectComboBox->currentText() != "None")
        return teamSelectComboBox->currentText();
    return "";
}

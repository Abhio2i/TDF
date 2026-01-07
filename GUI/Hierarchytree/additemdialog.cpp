
#include "additemdialog.h"
#include "core/Hierarchy/hierarchy.h"
#include "core/Hierarchy/EntityProfiles/radio.h"
#include "core/Hierarchy/EntityProfiles/sensor.h"
#include "core/Hierarchy/EntityProfiles/iff.h"
#include "core/Hierarchy/EntityProfiles/fixedpoints.h"
#include "core/Hierarchy/EntityProfiles/platform.h"
#include "core/Hierarchy/entity.h"

#include <QCheckBox>
#include <QComboBox>
#include <QDebug>
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

QString demangleComponentName(const std::string& mangledName) {
    QString name = QString::fromStdString(mangledName);

    // Remove leading digits
    while (!name.isEmpty() && name[0].isDigit()) {
        name.remove(0, 1);
    }

    // Make first letter capital if it's not empty
    if (!name.isEmpty()) {
        // First, check if it's already all caps (like "ID")
        bool isAllCaps = true;
        for (int i = 0; i < name.length(); ++i) {
            if (name[i].isLetter() && name[i].isLower()) {
                isAllCaps = false;
                break;
            }
        }

        if (!isAllCaps) {
            // Make only first letter capital for normal words
            name[0] = name[0].toUpper();
        }
    }

    return name;
}

/* Convert string to camelCase */
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
            // Keep as is (camelCase)
        }
        else if (result[i].isUpper() && result[i-1].isUpper()) {
            result[i] = result[i].toLower();
        }
    }
    return result;
}

/* Get default item name */
QString getDefaultName(AddItemDialog::DialogType type) {
    return (type == AddItemDialog::EntityType) ? "Entity" : "Folder";
}

AddItemDialog::AddItemDialog(DialogType type,
                             const QString &specificType,
                             DialogMode dialogMode,
                             Hierarchy* hierarchy,
                             QWidget *parent)
    : QDialog(parent),
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
    specificType(specificType),
    m_dialogMode(dialogMode),
    m_hierarchy(hierarchy)
{
    setupUI(type);
}

/* Helper method to create min-max spinbox pair */
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
    layout->addWidget(new QLabel(label + ":", this));

    minSpinBox = new QSpinBox(this);
    minSpinBox->setRange(minRange, maxRange);
    minSpinBox->setValue(minDefault);
    minSpinBox->setSuffix(unit);
    layout->addWidget(new QLabel("Min:", this));
    layout->addWidget(minSpinBox);

    maxSpinBox = new QSpinBox(this);
    maxSpinBox->setRange(minRange, maxRange);
    maxSpinBox->setValue(maxDefault);
    maxSpinBox->setSuffix(unit);
    layout->addWidget(new QLabel("Max:", this));
    layout->addWidget(maxSpinBox);

    layout->addStretch();
    return layout;
}

/* Setup Scenarioconfig section */
void AddItemDialog::setupScSection()
{
    // Main Scenarioconfig checkbox
    scCheckBox = new QCheckBox("Scenarioconfig", this);
    scCheckBox->setChecked(false);
    connect(scCheckBox, &QCheckBox::stateChanged, this, &AddItemDialog::onScCheckBoxStateChanged);

    // Create container widget for Scenarioconfig options
    scOptionsGroup = new QGroupBox("Scenarioconfig Options", this);
    QVBoxLayout *groupLayout = new QVBoxLayout();

    // Type selection (spread/circle)
    QHBoxLayout *typeLayout = new QHBoxLayout();
    typeLayout->addWidget(new QLabel("Type:", this));
    scTypeComboBox = new QComboBox(this);
    scTypeComboBox->addItems({"Spread", "Circle"});
    scTypeComboBox->setCurrentText("Spread");
    typeLayout->addWidget(scTypeComboBox);
    groupLayout->addLayout(typeLayout);

    // Range input with default value 100
    QHBoxLayout *rangeLayout = new QHBoxLayout();
    rangeLayout->addWidget(new QLabel("Range:", this));
    rangeLineEdit = new QLineEdit(this);
    rangeLineEdit->setText("100");
    rangeLineEdit->setPlaceholderText("Enter integer value");
    rangeLineEdit->setValidator(new QIntValidator(0, 1000000, this));
    rangeLayout->addWidget(rangeLineEdit);
    rangeLayout->addWidget(new QLabel(" km", this));
    groupLayout->addLayout(rangeLayout);

    // Add separator
    QFrame *separator1 = new QFrame(this);
    separator1->setFrameShape(QFrame::HLine);
    separator1->setFrameShadow(QFrame::Sunken);
    groupLayout->addWidget(separator1);

    // Min-Max Radio Range (10-100 m default)
    groupLayout->addLayout(createMinMaxSpinBoxPair("Radio Range",
                                                   minRadioRangeSpinBox, maxRadioRangeSpinBox,
                                                   10, 100, 1, 10000));

    // Min-Max Radar Range (50-500 m default)
    groupLayout->addLayout(createMinMaxSpinBoxPair("Radar Range",
                                                   minRadarRangeSpinBox, maxRadarRangeSpinBox,
                                                   50, 500, 1, 10000));

    // Add separator
    QFrame *separator2 = new QFrame(this);
    separator2->setFrameShape(QFrame::HLine);
    separator2->setFrameShadow(QFrame::Sunken);
    groupLayout->addWidget(separator2);

    // Trajectory dropdown (line/zigzag)
    QHBoxLayout *trajectoryLayout = new QHBoxLayout();
    trajectoryLayout->addWidget(new QLabel("Trajectory:", this));
    trajectoryComboBox = new QComboBox(this);
    trajectoryComboBox->addItems({"Line", "Zigzag"});
    trajectoryComboBox->setCurrentText("Line");
    trajectoryLayout->addWidget(trajectoryComboBox);
    trajectoryLayout->addStretch();
    groupLayout->addLayout(trajectoryLayout);

    // Add separator
    QFrame *separator3 = new QFrame(this);
    separator3->setFrameShape(QFrame::HLine);
    separator3->setFrameShadow(QFrame::Sunken);
    groupLayout->addWidget(separator3);

    // Min-Max Plane Speed (100-500 m/s default)
    QHBoxLayout *speedLayout = new QHBoxLayout();
    speedLayout->addWidget(new QLabel("Plane Speed:", this));

    minPlaneSpeedSpinBox = new QSpinBox(this);
    minPlaneSpeedSpinBox->setRange(0, 1000);
    minPlaneSpeedSpinBox->setValue(100);
    minPlaneSpeedSpinBox->setSuffix(" km/h");
    speedLayout->addWidget(new QLabel("Min:", this));
    speedLayout->addWidget(minPlaneSpeedSpinBox);

    maxPlaneSpeedSpinBox = new QSpinBox(this);
    maxPlaneSpeedSpinBox->setRange(0, 8000);
    maxPlaneSpeedSpinBox->setValue(500);
    maxPlaneSpeedSpinBox->setSuffix(" km/h");
    speedLayout->addWidget(new QLabel("Max:", this));
    speedLayout->addWidget(maxPlaneSpeedSpinBox);

    speedLayout->addStretch();
    groupLayout->addLayout(speedLayout);

    // Min-Max Turn Radius (50-200 m default)
    groupLayout->addLayout(createMinMaxSpinBoxPair("Turn Radius",
                                                   minTurnRadiusSpinBox, maxTurnRadiusSpinBox,
                                                   50, 200, 1, 1000, " m"));

    // Add some spacing at the bottom
    groupLayout->addSpacing(10);

    scOptionsGroup->setLayout(groupLayout);
    scOptionsGroup->setVisible(false);
}

/* Handle scenarioconfig checkbox state change */
void AddItemDialog::onScCheckBoxStateChanged(int state)
{
    bool isChecked = (state == Qt::Checked);

    if (scOptionsGroup) {
        scOptionsGroup->setVisible(isChecked);
    }
}


void AddItemDialog::setupUI(DialogType type)
{
    QWidget *mainWidget = new QWidget(this);
    QVBoxLayout *mainLayout = new QVBoxLayout(mainWidget);

    QString itemType = (type == EntityType) ? "Entity" : "Folder";

    bool isProfileSensorAdd = (specificType == "Sensor");
    bool isComponentSensorAdd = (m_dialogMode == ComponentSensorMode) || (specificType == "sensors");
    bool isComponentIFFAdd = (m_dialogMode == ComponentIFFMode) || (specificType == "iffs");
    bool isComponentRadioAdd = (m_dialogMode == ComponentRadioMode) || (specificType == "radios");

    bool isForComponentAdd = isComponentSensorAdd || isComponentIFFAdd || isComponentRadioAdd;
    bool isForSensor = isProfileSensorAdd || isComponentSensorAdd;

    // ✅ सबसे ऊपर: Select Profile dropdown (केवल Component Add mode में)
    if (isForComponentAdd) {
        QHBoxLayout *selectProfileLayout = new QHBoxLayout();

        profileComboBox = new QComboBox(this);
        profileComboBox->setEditable(false);

        QString placeholderText;
        if (isComponentSensorAdd) placeholderText = "Select Sensor";
        else if (isComponentIFFAdd) placeholderText = "Select IFF";
        else if (isComponentRadioAdd) placeholderText = "Select Radio";

        // Dummy placeholder item
        profileComboBox->addItem(placeholderText);
        profileComboBox->setCurrentIndex(0);

        // Placeholder style: gray + italic
        QFont placeholderFont = profileComboBox->font();
        placeholderFont.setItalic(true);
        profileComboBox->setItemData(0, QColor(Qt::gray), Qt::TextColorRole);
        profileComboBox->setItemData(0, placeholderFont, Qt::FontRole);

        // Real profiles load based on type
        if (isComponentSensorAdd) {
            populateSensorProfiles();
        } else if (isComponentIFFAdd) {
            populateIFFProfiles();
        } else if (isComponentRadioAdd) {
            populateRadioProfiles();
        }
        connect(profileComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged),
                this, [=](int index) {
                    if (index > 0) {
                        QString selectedName = profileComboBox->currentText();
                        QString selectedId = profileComboBox->currentData().toString();
                        QString profileType;

                        if (isComponentSensorAdd) profileType = "Sensor";
                        else if (isComponentIFFAdd) profileType = "IFF";
                        else if (isComponentRadioAdd) profileType = "Radio";
                    }
                });
        profileComboBox->setMinimumWidth(250);

        selectProfileLayout->addWidget(profileComboBox);
        selectProfileLayout->addStretch();
        mainLayout->addLayout(selectProfileLayout);
    }


    QHBoxLayout *nameLayout = new QHBoxLayout();
    nameLayout->addWidget(new QLabel(itemType + " Name:", this));
    nameLineEdit = new QLineEdit(this);
    QString defaultName = isForSensor ? "Sensor" : getDefaultName(type);

    // Default name based on component type
    if (isComponentIFFAdd) defaultName = "IFF";
    else if (isComponentRadioAdd) defaultName = "Radio";

    nameLineEdit->setText(defaultName);
    nameLineEdit->setPlaceholderText("Enter " + itemType + " name");
    nameLayout->addWidget(nameLineEdit);
    mainLayout->addLayout(nameLayout);


    if (isForSensor) {
        QHBoxLayout *sensorTypeLayout = new QHBoxLayout();
        sensorTypeLayout->addWidget(new QLabel("Sensor Type:", this));
        sensorTypeComboBox = new QComboBox(this);
        sensorTypeComboBox->addItems({"Generic", "CSM", "ESM"});
        sensorTypeComboBox->setCurrentText("Generic");
        sensorTypeLayout->addWidget(sensorTypeComboBox);
        sensorTypeLayout->addStretch();
        mainLayout->addLayout(sensorTypeLayout);
    }


    if (type == EntityType && !isForComponentAdd && !isForSensor) {
        QHBoxLayout *numberLayout = new QHBoxLayout();
        numberLayout->addWidget(new QLabel("Number of Entities:", this));
        numberLineEdit = new QLineEdit(this);
        numberLineEdit->setText("1");
        numberLineEdit->setPlaceholderText("Enter number (default: 1)");
        numberLineEdit->setValidator(new QIntValidator(1, 10000, this));
        numberLayout->addWidget(numberLineEdit);
        mainLayout->addLayout(numberLayout);
    }


    bool isPlatformProfile = (type == EntityType && !isForComponentAdd && !isForSensor &&
                              (specificType.isEmpty() || specificType == "Platform"));
    if (isPlatformProfile) {
        setupScSection();
        mainLayout->addWidget(scCheckBox);
        mainLayout->addWidget(scOptionsGroup);
    }


    bool shouldShowComponents = (type == EntityType && !isForComponentAdd && !isForSensor &&
                                 (specificType.isEmpty() || specificType == "Platform" || specificType == "SpecialZone"|| specificType == "FixedPoints" || specificType == "Entity"));

    if (shouldShowComponents) {
        QGroupBox *componentsGroup = new QGroupBox("Components", this);
        QVBoxLayout *componentsLayout = new QVBoxLayout();

        Entity *entity = nullptr;
        QString effectiveType = specificType.isEmpty() ? "Entity" : specificType;

        if (effectiveType == "Platform") {
            entity = new Platform(nullptr);
        } else
        if (effectiveType == "FixedPoints") {
            entity = new FixedPoints(nullptr);
        } else
        if (effectiveType == "SpecialZone") {
            entity = new Specialzone(nullptr);
        } else
        {
            entity = new Platform(nullptr);
        }

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
        mainLayout->addWidget(componentsGroup);
        delete entity;
    }

    mainLayout->addStretch();

    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    mainLayout->addWidget(buttonBox);


    bool isSimpleDialog = (specificType == "Formation" ||
                           specificType == "IFF" ||
                           specificType == "Radio" ||
                           isProfileSensorAdd ||
                           isComponentSensorAdd ||
                           isComponentIFFAdd ||
                           isComponentRadioAdd);

    if (isSimpleDialog) {
        // ✅ Simple layout for Formation, Radio, IFF, FixedPoints, Sensors
        QVBoxLayout *dialogLayout = new QVBoxLayout(this);
        dialogLayout->addWidget(mainWidget);
        setLayout(dialogLayout);

        // Adjust size to content
        mainWidget->adjustSize();
        adjustSize();

        // Set fixed size based on type
        QString windowTitle;
        if (isComponentSensorAdd) {
            windowTitle = "Add Sensors";
            setFixedSize(350, 180);
        } else if (isComponentIFFAdd) {
            windowTitle = "Add IFFs";
            setFixedSize(350, 180);
        } else if (isComponentRadioAdd) {
            windowTitle = "Add Radios";
            setFixedSize(350, 180);
        } else if (isProfileSensorAdd) {
            windowTitle = "Add Sensor";
            setFixedSize(320, 150);
        } else if (specificType == "Formation" ||
                   specificType == "FixedPoints" ||
                   specificType == "IFF" ||
                   specificType == "Radio") {
            windowTitle = "Add " + specificType;
            setFixedSize(350, 150);
        } else {
            windowTitle = "Add " + specificType;
            setFixedSize(350, 200);
        }

        setWindowTitle(windowTitle);
    } else {
        // ✅ Scrollable layout for Platform and complex dialogs
        QScrollArea *scrollArea = new QScrollArea(this);
        scrollArea->setWidgetResizable(true);
        scrollArea->setWidget(mainWidget);
        scrollArea->setFrameShape(QFrame::NoFrame);

        QVBoxLayout *dialogLayout = new QVBoxLayout(this);
        dialogLayout->addWidget(scrollArea);
        setLayout(dialogLayout);

        QString windowTitle = "Add " + (specificType.isEmpty() ? itemType : specificType);


        if (specificType == "Platform" || specificType.isEmpty()) {
            setFixedSize(500, 650);
        } else {

            setFixedSize(450, 400);
        }

        setWindowTitle(windowTitle);

        scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
        scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    }


    if (nameLineEdit) {
        nameLineEdit->setFocus();
        nameLineEdit->selectAll();
    }
}

QString AddItemDialog::getName() const {
    return nameLineEdit ? nameLineEdit->text().trimmed() : "";
}

int AddItemDialog::getNumber() const {
    if (numberLineEdit && !numberLineEdit->text().trimmed().isEmpty()) {
        return numberLineEdit->text().trimmed().toInt();
    }
    return 1;
}


QVariantMap AddItemDialog::getComponents() const {
    QVariantMap components;
    for (auto it = componentCheckboxes.begin(); it != componentCheckboxes.end(); ++it) {
        components.insert(it.key(), it.value()->isChecked());
    }
    return components;
}

QString AddItemDialog::getSensorType() const {
    return sensorTypeComboBox ? sensorTypeComboBox->currentText() : "Generic";
}

QString AddItemDialog::getProfileId() const {
    if (profileComboBox && profileComboBox->currentIndex() > 0) {
        return profileComboBox->currentData().toString();
    }
    return "";
}

QString AddItemDialog::getProfileName() const {
    if (profileComboBox && profileComboBox->currentIndex() > 0) {
        return profileComboBox->currentText();
    }
    return "";
}

QString AddItemDialog::getComponentType() const {
    if (m_dialogMode == ComponentSensorMode) return "sensors";
    if (m_dialogMode == ComponentIFFMode) return "iffs";
    if (m_dialogMode == ComponentRadioMode) return "radios";
    return "";
}

// Generic function to populate profiles
void AddItemDialog::populateProfiles(const QString& profileType)
{
    if (!profileComboBox || !m_hierarchy) {
        qDebug() << "❌ No profileComboBox or hierarchy available";
        return;
    }

    qDebug() << "=== POPULATING" << profileType << "PROFILES ===";

    bool foundProfile = false;

    for (const auto& [profileId, profile] : m_hierarchy->ProfileCategories) {
        if (!profile) continue;

        QString profileName = QString::fromStdString(profile->Name);

        if (profileName == profileType) {
            foundProfile = true;
            qDebug() << "✅ Found" << profileType << "profile! Entities count:" << profile->Entities.size();

            if (profile->Entities.empty()) {
                qDebug() << "No entities in" << profileType << "profile";
                return;
            }

            // Collect and sort by name
            QMap<QString, QString> profileEntities;

            for (const auto& [entityId, entity] : profile->Entities) {
                if (!entity) continue;
                QString entityIdStr = QString::fromStdString(entityId);
                QString entityName = QString::fromStdString(entity->Name);

                if (!entityName.isEmpty()) {
                    profileEntities.insert(entityName, entityIdStr);
                }
            }

            // Add in alphabetical order
            for (auto it = profileEntities.begin(); it != profileEntities.end(); ++it) {
                profileComboBox->addItem(it.key(), it.value());
                qDebug() << "Added to dropdown:" << it.key() << "(ID:" << it.value() << ")";
            }

            qDebug() << "Total" << profileType << "added to dropdown:" << profileEntities.size();
            return;
        }
    }

    if (!foundProfile) {
        qDebug() << profileType << "profile not found in hierarchy";
    }
}

// Specific functions for each profile type
void AddItemDialog::populateSensorProfiles()
{
    populateProfiles("Sensor");
}

void AddItemDialog::populateIFFProfiles()
{
    populateProfiles("IFF");
}

void AddItemDialog::populateRadioProfiles()
{
    populateProfiles("Radio");
}

// Scenarioconfig getters
bool AddItemDialog::isScenarioconfigEnabled() const {
    return scCheckBox ? scCheckBox->isChecked() : false;
}

QString AddItemDialog::getScType() const {
    return scTypeComboBox ? scTypeComboBox->currentText() : "Spread";
}

int AddItemDialog::getScRange() const {
    return rangeLineEdit ? rangeLineEdit->text().toInt() : 100;
}

int AddItemDialog::getMinRadioRange() const {
    return minRadioRangeSpinBox ? minRadioRangeSpinBox->value() : 10;
}

int AddItemDialog::getMaxRadioRange() const {
    return maxRadioRangeSpinBox ? maxRadioRangeSpinBox->value() : 100;
}

int AddItemDialog::getMinRadarRange() const {
    return minRadarRangeSpinBox ? minRadarRangeSpinBox->value() : 50;
}

int AddItemDialog::getMaxRadarRange() const {
    return maxRadarRangeSpinBox ? maxRadarRangeSpinBox->value() : 500;
}

QString AddItemDialog::getTrajectory() const {
    return trajectoryComboBox ? trajectoryComboBox->currentText() : "Line";
}

int AddItemDialog::getMinPlaneSpeed() const {
    return minPlaneSpeedSpinBox ? minPlaneSpeedSpinBox->value() : 100;
}

int AddItemDialog::getMaxPlaneSpeed() const {
    return maxPlaneSpeedSpinBox ? maxPlaneSpeedSpinBox->value() : 500;
}

int AddItemDialog::getMinTurnRadius() const {
    return minTurnRadiusSpinBox ? minTurnRadiusSpinBox->value() : 50;
}

int AddItemDialog::getMaxTurnRadius() const {
    return maxTurnRadiusSpinBox ? maxTurnRadiusSpinBox->value() : 200;
}

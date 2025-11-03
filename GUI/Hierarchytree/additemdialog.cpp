

/* ========================================================================= */
/* File: additemdialog.cpp                                                  */
/* Purpose: Implements dialog for adding entities or folders with components */
/* ========================================================================= */

#include "GUI/Hierarchytree/additemdialog.h"
#include "core/Hierarchy/entity.h"
#include "core/Hierarchy/EntityProfiles/platform.h"
#include "core/Hierarchy/EntityProfiles/radio.h"
#include "core/Hierarchy/EntityProfiles/sensor.h"
#include "core/Hierarchy/EntityProfiles/fixedpoints.h"
#include "core/Hierarchy/EntityProfiles/iff.h"
#include <QDebug>
#include <QCheckBox>
#include <QGroupBox>
#include <QLabel>
#include <QLineEdit>
#include <QDialogButtonBox>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QValidator>
#include <QComboBox>

// %%% Utility Functions %%%
/* Demangle component names */
QString demangleComponentName(const std::string& mangledName) {
    QString name = QString::fromStdString(mangledName);
    while (!name.isEmpty() && name[0].isDigit()) {
        name.remove(0, 1);
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

// %%% Constructor %%%
/* Initialize add item dialog */
AddItemDialog::AddItemDialog(DialogType type, const QString &specificType, QWidget *parent)
    : QDialog(parent), specificType(specificType), sensorTypeComboBox(nullptr)
{
    setupUI(type);
}

/* Setup dialog UI */
void AddItemDialog::setupUI(DialogType type)
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    // Name input section
    QHBoxLayout *nameLayout = new QHBoxLayout();
    QString itemType = (type == EntityType) ? "Entity" : "Folder";
    nameLayout->addWidget(new QLabel(itemType + " Name:", this));
    nameLineEdit = new QLineEdit(this);
    QString defaultName = getDefaultName(type);
    nameLineEdit->setText(defaultName);
    nameLineEdit->setPlaceholderText("Enter " + itemType + " name");
    nameLayout->addWidget(nameLineEdit);
    mainLayout->addLayout(nameLayout);
    //->CHANGE
    // Sensor type dropdown (only for sensors)
    if (specificType.toLower() == "sensors") {
        QHBoxLayout *sensorTypeLayout = new QHBoxLayout();
        sensorTypeLayout->addWidget(new QLabel("Sensor Type:", this));
        sensorTypeComboBox = new QComboBox(this);
        sensorTypeComboBox->addItems({"Generic", "CSM", "ESM"}); // ✅ cleaner naming
        sensorTypeComboBox->setCurrentText("Generic"); // ✅ default
        sensorTypeLayout->addWidget(sensorTypeComboBox);
        mainLayout->addLayout(sensorTypeLayout);
    }

    // Number input section (for EntityType)
    if (type == EntityType && specificType.toLower() != "sensors") {
        QHBoxLayout *numberLayout = new QHBoxLayout();
        numberLayout->addWidget(new QLabel("Number of Entities:", this));
        numberLineEdit = new QLineEdit(this);
        numberLineEdit->setText("1");
        numberLineEdit->setPlaceholderText("Enter number (default: 1)");
        QIntValidator *validator = new QIntValidator(1, 10000, this);
        numberLineEdit->setValidator(validator);
        numberLayout->addWidget(numberLineEdit);
        mainLayout->addLayout(numberLayout);
    }

    // Components section (for EntityType, excluding sensors)
    if (type == EntityType && specificType.toLower() != "sensors") {
        QGroupBox *componentsGroup = new QGroupBox("Components", this);
        QVBoxLayout *componentsLayout = new QVBoxLayout();
        Entity *entity = nullptr;
        QString effectiveType = specificType.isEmpty() ? "Entity" : specificType;
        if (effectiveType == "Radio") {
            entity = new Radio(nullptr);
        } else if (effectiveType == "Sensor") {
            entity = new Sensor(nullptr);
        } else if (effectiveType == "FixedPoints") {
            entity = new FixedPoints(nullptr);
        } else if (effectiveType == "IFF" || effectiveType == "Formation") {
            entity = new IFF(nullptr);
        } else {
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
            qDebug() << "Adding component to UI:" << displayName << "for specificType:" << effectiveType;
            QCheckBox *checkBox = new QCheckBox(displayName, this);
            if (camelCaseName == "transform") {
                checkBox->setChecked(true);
                checkBox->setEnabled(false);
                qDebug() << "Transform component added (mandatory) for specificType:" << effectiveType;
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

    // Dialog buttons
    QDialogButtonBox *buttonBox = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    mainLayout->addWidget(buttonBox);
    setLayout(mainLayout);
    setWindowTitle("Add " + (specificType.toLower() == "sensors" ? "Sensor" : itemType));
    resize(300, specificType.toLower() == "sensors" ? 200 : 300);
}

/* Get entered name */
QString AddItemDialog::getName() const {
    return nameLineEdit->text().trimmed();
}

/* Get number of entities */
int AddItemDialog::getNumber() const {
    return numberLineEdit ? numberLineEdit->text().trimmed().toInt() : 1;
}

/* Get selected components */
QVariantMap AddItemDialog::getComponents() const {
    QVariantMap components;
    for (auto it = componentCheckboxes.begin(); it != componentCheckboxes.end(); ++it) {
        components.insert(it.key(), it.value()->isChecked());
    }
    return components;
}

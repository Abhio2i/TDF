/* ========================================================================= */
/* File: additemdialog.cpp                                                  */
/* Purpose: Implements dialog for adding entities or folders with components */
/* ========================================================================= */

#include "GUI/Hierarchytree/additemdialog.h"        // For add item dialog class
#include "core/Hierarchy/entity.h"                  // For entity base class
#include "core/Hierarchy/EntityProfiles/platform.h"  // For platform entity profile
#include "core/Hierarchy/EntityProfiles/radio.h"    // For radio entity profile
#include "core/Hierarchy/EntityProfiles/sensor.h"   // For sensor entity profile
#include "core/Hierarchy/EntityProfiles/fixedpoints.h" // For fixed points profile
#include "core/Hierarchy/EntityProfiles/iff.h"      // For IFF entity profile
#include <QDebug>                                  // For debug output
#include <QCheckBox>                               // For checkbox widget
#include <QGroupBox>                               // For group box widget
#include <QLabel>                                  // For label widget
#include <QLineEdit>                               // For text input widget
#include <QDialogButtonBox>                        // For dialog buttons
#include <QVBoxLayout>                             // For vertical layout
#include <QHBoxLayout>                             // For horizontal layout
#include <QValidator>                              // For input validation

// %%% Utility Functions %%%
/* Demangle component names */
QString demangleComponentName(const std::string& mangledName) {
    // Convert string to QString
    QString name = QString::fromStdString(mangledName);
    // Remove leading digits
    while (!name.isEmpty() && name[0].isDigit()) {
        name.remove(0, 1);
    }
    return name;
}

/* Convert string to camelCase */
QString toCamelCase(const QString& input) {
    // Return empty string if input is empty
    if (input.isEmpty()) return input;
    // Initialize result
    QString result = input;
    // Lowercase first character
    if (!result.isEmpty()) {
        result[0] = result[0].toLower();
    }
    // Process characters for camelCase
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
    // Return default name based on type
    return (type == AddItemDialog::EntityType) ? "Entity" : "Folder";
}

// %%% Constructor %%%
/* Initialize add item dialog */
AddItemDialog::AddItemDialog(DialogType type, const QString &specificType, QWidget *parent)
    : QDialog(parent), specificType(specificType)
{
    // Setup dialog UI
    setupUI(type);
}

/* Setup dialog UI */
void AddItemDialog::setupUI(DialogType type)
{
    // Create main vertical layout
    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    // Name input section
    QHBoxLayout *nameLayout = new QHBoxLayout();
    QString itemType = (type == EntityType) ? "Entity" : "Folder";
    nameLayout->addWidget(new QLabel(itemType + " Name:", this));
    nameLineEdit = new QLineEdit(this);

    // Set default name
    QString defaultName = getDefaultName(type);
    nameLineEdit->setText(defaultName);
    nameLineEdit->setPlaceholderText("Enter " + itemType + " name");

    nameLayout->addWidget(nameLineEdit);
    mainLayout->addLayout(nameLayout);

    // Number input section (for EntityType)
    if (type == EntityType) {
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

    // Components section (for EntityType)
    if (type == EntityType) {
        QGroupBox *componentsGroup = new QGroupBox("Components", this);
        QVBoxLayout *componentsLayout = new QVBoxLayout();

        // Create entity based on specificType
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

        // Get supported components
        std::vector<std::string> supportedComponents = entity->getSupportedComponents();
        QMap<QString, Component*> uniqueComponents;
        for (const std::string &component : supportedComponents) {
            uniqueComponents.insert(demangleComponentName(component), nullptr);
        }
        // Add component checkboxes
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
    // Set dialog properties
    setLayout(mainLayout);
    setWindowTitle("Add " + itemType);
    resize(300, 300);
}

/* Get entered name */
QString AddItemDialog::getName() const {
    // Return trimmed name
    return nameLineEdit->text().trimmed();
}

/* Get number of entities */
int AddItemDialog::getNumber() const {
    // Return number as integer
    return numberLineEdit->text().trimmed().toInt();
}

/* Get selected components */
QVariantMap AddItemDialog::getComponents() const {
    // Collect checked components
    QVariantMap components;
    for (auto it = componentCheckboxes.begin(); it != componentCheckboxes.end(); ++it) {
        components.insert(it.key(), it.value()->isChecked());
    }
    return components;
}

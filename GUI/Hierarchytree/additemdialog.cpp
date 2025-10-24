
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

// Helper function to demangle component names
QString demangleComponentName(const std::string& mangledName) {
    QString name = QString::fromStdString(mangledName);
    while (!name.isEmpty() && name[0].isDigit()) {
        name.remove(0, 1);
    }
    return name;
}

// Helper function to convert to camelCase
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

// Helper function to get default name (plain itemType without numbering)
QString getDefaultName(AddItemDialog::DialogType type) {
    return (type == AddItemDialog::EntityType) ? "Entity" : "Folder";
}

AddItemDialog::AddItemDialog(DialogType type, const QString &specificType, QWidget *parent)
    : QDialog(parent), specificType(specificType)
{
    setupUI(type);
}

void AddItemDialog::setupUI(DialogType type)
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    // Name input
    QHBoxLayout *nameLayout = new QHBoxLayout();
    QString itemType = (type == EntityType) ? "Entity" : "Folder";
    nameLayout->addWidget(new QLabel(itemType + " Name:", this));
    nameLineEdit = new QLineEdit(this);

    // Set default name (plain itemType)
    QString defaultName = getDefaultName(type);
    nameLineEdit->setText(defaultName);
    nameLineEdit->setPlaceholderText("Enter " + itemType + " name");

    nameLayout->addWidget(nameLineEdit);
    mainLayout->addLayout(nameLayout);

    // Number input (only for EntityType)
    if (type == EntityType) {
        QHBoxLayout *numberLayout = new QHBoxLayout();
        numberLayout->addWidget(new QLabel("Number of Entities:", this));
        numberLineEdit = new QLineEdit(this);
        numberLineEdit->setText("1");
        numberLineEdit->setPlaceholderText("Enter number (default: 1)");
        QIntValidator *validator = new QIntValidator(1, 10000, this);
        numberLineEdit->setValidator(validator);
        numberLayout->addWidget(numberLineEdit); // Fixed: Add numberLineEdit
        mainLayout->addLayout(numberLayout);
    }

    // Components section (only for EntityType)
    if (type == EntityType) {
        QGroupBox *componentsGroup = new QGroupBox("Components", this);
        QVBoxLayout *componentsLayout = new QVBoxLayout();

        // Create an instance of the appropriate entity based on specificType
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
            entity = new Platform(nullptr); // Fallback to Platform for components
        }

        // Get supported components for the selected entity type
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
                checkBox->setEnabled(false); // Disable Transform checkbox as it's mandatory
                qDebug() << "Transform component added (mandatory) for specificType:" << effectiveType;
            } else {
                checkBox->setChecked(true); // Default checked for other components
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
    setWindowTitle("Add " + itemType);
    resize(300, 300);
}

QString AddItemDialog::getName() const {
    return nameLineEdit->text().trimmed();
}

int AddItemDialog::getNumber() const {
    return numberLineEdit->text().trimmed().toInt();
}

QVariantMap AddItemDialog::getComponents() const {
    QVariantMap components;
    for (auto it = componentCheckboxes.begin(); it != componentCheckboxes.end(); ++it) {
        components.insert(it.key(), it.value()->isChecked());
    }
    return components;
}


#include "GUI/Hierarchytree/additemdialog.h"
#include "core/Hierarchy/entity.h"
#include "core/Hierarchy/EntityProfiles/platform.h"
#include <QDebug>
#include <QCheckBox>
#include <QGroupBox>
#include <QLabel>
#include <QLineEdit>
#include <QDialogButtonBox>
#include <QVBoxLayout>
#include <QHBoxLayout>

QString demangleComponentName(const std::string& mangledName) {
    QString name = QString::fromStdString(mangledName);


    while (!name.isEmpty() && name[0].isDigit()) {
        name.remove(0, 1);
    }

    return name;
}

QString toCamelCase(const QString& input) {
    if (input.isEmpty()) return input;

    QString result = input;

    // First character to lowercase
    if (!result.isEmpty()) {
        result[0] = result[0].toLower();
    }

    // Special handling for "2D" -> "2d"
    for (int i = 1; i < result.size(); ++i) {
        // If we find "2D" pattern
        if (result[i] == 'D' && i > 0 && result[i-1].isDigit()) {
            result[i] = result[i].toLower();
        }
        // Regular camelCase handling
        else if (result[i].isUpper() && result[i-1].isLower()) {
            // Keep as is (camelCase)
        }
        else if (result[i].isUpper() && result[i-1].isUpper()) {
            // Convert to lowercase for abbreviations
            result[i] = result[i].toLower();
        }
    }

    return result;
}

AddItemDialog::AddItemDialog(DialogType type, QWidget *parent)
    : QDialog(parent)
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
    nameLineEdit->setPlaceholderText("Enter " + itemType + " name");
    nameLayout->addWidget(nameLineEdit);
    mainLayout->addLayout(nameLayout);

    // Components section
    QGroupBox *componentsGroup = new QGroupBox("Components", this);
    QVBoxLayout *componentsLayout = new QVBoxLayout();

    // Create an instance of Entity to get supported components
    // Entity tempEntity(nullptr);
    // std::vector<Component*> supportedComponents = tempEntity.getSupportedComponents();
    std::vector<Component*> supportedComponents = {
        new Transform(),
        new MeshRenderer2D(),
        new DynamicModel(),
        new Rigidbody(),
        new Collider()
    };
    // Use a map to track unique components by their display name
    QMap<QString, Component*> uniqueComponents;

    // First pass to identify unique components
    for (Component* component : supportedComponents) {
        QString rawName = demangleComponentName(typeid(*component).name());
        QString displayName = rawName;

        // Only add if we haven't seen this component before
        if (!uniqueComponents.contains(displayName)) {
            uniqueComponents.insert(displayName, component);
        }
    }

    // Second pass to create checkboxes for unique components
    for (auto it = uniqueComponents.begin(); it != uniqueComponents.end(); ++it) {
        QString displayName = it.key();
        QString camelCaseName = toCamelCase(displayName);

        qDebug() << "Adding component to UI:" << displayName;
        QCheckBox *checkBox = new QCheckBox(displayName, this);

        // Special handling for Transform component
        if (camelCaseName == "transform") {
            checkBox->setChecked(true);
            checkBox->setEnabled(false); // Disable Transform checkbox as it's mandatory
            qDebug() << "Transform component added (mandatory)";
        } else {
            checkBox->setChecked(false); // Default unchecked for other components
        }

        componentCheckboxes.insert(camelCaseName, checkBox);
        componentsLayout->addWidget(checkBox);
    }

    componentsGroup->setLayout(componentsLayout);
    mainLayout->addWidget(componentsGroup);

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

QVariantMap AddItemDialog::getComponents() const {
    QVariantMap components;
    for (auto it = componentCheckboxes.begin(); it != componentCheckboxes.end(); ++it) {
        components.insert(it.key(), it.value()->isChecked());
    }
    return components;
}


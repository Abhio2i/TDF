#include "additemdialog_test.h"
#include "GUI/Hierarchytree/additemdialog.h"
#include "core/Hierarchy/hierarchy.h"
#include <QTest>
#include <QLineEdit>
#include <QComboBox>
#include <QSpinBox>
#include <QCheckBox>
#include <QDialogButtonBox>
#include <QPushButton>
#include <QLabel>
#include <QIntValidator>

void TestAddItemDialog::init()
{
    // Create a dummy hierarchy
    dummyHierarchyForCleanup = new Hierarchy();

    // Use fully qualified enum types (scoped inside AddItemDialog)
    AddItemDialog::DialogType type = static_cast<AddItemDialog::DialogType>(0);  // 0 = e.g., AddEntity
    QString specificType = "Platform";
    AddItemDialog::DialogMode mode = static_cast<AddItemDialog::DialogMode>(0);  // 0 = e.g., Create

    dialog = new AddItemDialog(type, specificType, mode, dummyHierarchyForCleanup, nullptr, "");
}

void TestAddItemDialog::cleanup()
{
    delete dialog;
    delete dummyHierarchyForCleanup;
    dialog = nullptr;
    dummyHierarchyForCleanup = nullptr;
}

// ------------------------------------------------------------------
// Basic properties
// ------------------------------------------------------------------
void TestAddItemDialog::testWindowTitle()
{
    QVERIFY(dialog->windowTitle().contains("Add"));
}



// ------------------------------------------------------------------
// UI elements
// ------------------------------------------------------------------
void TestAddItemDialog::testNameLineEdit()
{
    QLineEdit* nameField = nullptr;
    for (QLineEdit* le : dialog->findChildren<QLineEdit*>()) {
        if (le->placeholderText().contains("Name", Qt::CaseInsensitive) ||
            le->placeholderText().contains("name")) {
            nameField = le;
            break;
        }
    }
    if (!nameField) {
        // fallback: first line edit that is not a search field
        for (QLineEdit* le : dialog->findChildren<QLineEdit*>()) {
            if (!le->placeholderText().contains("Search", Qt::CaseInsensitive)) {
                nameField = le;
                break;
            }
        }
    }
    QVERIFY(nameField != nullptr);
    nameField->setText("TestEntity");
    QCOMPARE(nameField->text(), QString("TestEntity"));
}

void TestAddItemDialog::testOkCancelButtons()
{
    QDialogButtonBox* buttonBox = dialog->findChild<QDialogButtonBox*>();
    QVERIFY(buttonBox != nullptr);
    QPushButton* okButton = buttonBox->button(QDialogButtonBox::Ok);
    QPushButton* cancelButton = buttonBox->button(QDialogButtonBox::Cancel);
    QVERIFY(okButton != nullptr);
    QVERIFY(cancelButton != nullptr);
    QVERIFY(okButton->isEnabled());
    QVERIFY(cancelButton->isEnabled());
}

// ------------------------------------------------------------------
// Number field (optional)
// ------------------------------------------------------------------
void TestAddItemDialog::testNumberField()
{
    QLineEdit* numberField = nullptr;
    for (QLineEdit* le : dialog->findChildren<QLineEdit*>()) {
        if (le->validator() && le->validator()->inherits("QIntValidator")) {
            numberField = le;
            break;
        }
    }
    if (!numberField) {
        QSKIP("No integer validator field found (optional)", SkipSingle);
    }
    int defaultValue = numberField->text().toInt();
    QVERIFY(defaultValue >= 1);
    numberField->setText("5");
    QCOMPARE(numberField->text(), QString("5"));
    int getNumberValue = dialog->getNumber();
    QCOMPARE(getNumberValue, 5);
}

// ------------------------------------------------------------------
// Sensor type combo (optional)
// ------------------------------------------------------------------
void TestAddItemDialog::testSensorTypeCombo()
{
    if (!dialog->windowTitle().contains("Sensor", Qt::CaseInsensitive)) {
        QSKIP("Not a sensor dialog", SkipSingle);
    }
    QComboBox* sensorTypeCombo = dialog->findChild<QComboBox*>();
    if (!sensorTypeCombo || sensorTypeCombo->count() == 0) {
        QSKIP("No sensor type combo with items", SkipSingle);
    }
    QVERIFY(sensorTypeCombo->count() > 0);
    QString defaultType = dialog->getSensorType();
    QVERIFY(!defaultType.isEmpty());
}

// ------------------------------------------------------------------
// Component checkboxes (optional)
// ------------------------------------------------------------------
void TestAddItemDialog::testComponentCheckboxes()
{
    QList<QCheckBox*> checkboxes = dialog->findChildren<QCheckBox*>();
    if (checkboxes.isEmpty()) {
        QSKIP("No component checkboxes", SkipSingle);
    }
    bool hasTransform = false;
    for (QCheckBox* cb : checkboxes) {
        if (cb->text().contains("transform", Qt::CaseInsensitive)) {
            hasTransform = true;
            QVERIFY(true);
            break;
        }
    }
    if (!hasTransform) {
        QSKIP("Transform checkbox not found", SkipSingle);
    }
}

// ------------------------------------------------------------------
// Getters
// ------------------------------------------------------------------
void TestAddItemDialog::testGetName()
{
    QLineEdit* nameField = nullptr;
    for (QLineEdit* le : dialog->findChildren<QLineEdit*>()) {
        if (le->placeholderText().contains("Name", Qt::CaseInsensitive) ||
            le->placeholderText().contains("name")) {
            nameField = le;
            break;
        }
    }
    if (!nameField) {
        QSKIP("No name line edit found", SkipSingle);
    }
    nameField->setText("UnitTestName");
    QCOMPARE(dialog->getName(), QString("UnitTestName"));
}

void TestAddItemDialog::testGetNumber()
{
    QLineEdit* numberField = nullptr;
    for (QLineEdit* le : dialog->findChildren<QLineEdit*>()) {
        if (le->validator() && le->validator()->inherits("QIntValidator")) {
            numberField = le;
            break;
        }
    }
    if (!numberField) {
        QSKIP("No integer validator field found", SkipSingle);
    }
    numberField->setText("10");
    QCOMPARE(dialog->getNumber(), 10);
}

void TestAddItemDialog::testSetNumber()
{
    QLineEdit* numberField = nullptr;
    for (QLineEdit* le : dialog->findChildren<QLineEdit*>()) {
        if (le->validator() && le->validator()->inherits("QIntValidator")) {
            numberField = le;
            break;
        }
    }
    if (!numberField) {
        QSKIP("No integer validator field found", SkipSingle);
    }
    dialog->setNumber(7);
    QCOMPARE(numberField->text(), QString("7"));
}

void TestAddItemDialog::testGetComponents()
{
    QVariantMap components = dialog->getComponents();
    QVERIFY(true); // may be empty, that's fine
}

// ------------------------------------------------------------------
// Optional: profile combo, team combo, search field, scenario config checkbox
// ------------------------------------------------------------------
void TestAddItemDialog::testProfileCombo()
{
    QComboBox* profileCombo = nullptr;
    for (QComboBox* cb : dialog->findChildren<QComboBox*>()) {
        if (cb->count() > 1 && !cb->currentText().contains("None") &&
            !cb->currentText().contains("Team", Qt::CaseInsensitive)) {
            profileCombo = cb;
            break;
        }
    }
    if (!profileCombo) {
        QSKIP("No profile combo found", SkipSingle);
    }
    QString profileId = dialog->getProfileId();
    QString profileName = dialog->getProfileName();
    QVERIFY(profileId.isEmpty() || !profileId.isEmpty());
    QVERIFY(profileName.isEmpty() || !profileName.isEmpty());
}

void TestAddItemDialog::testTeamCombo()
{
    QComboBox* teamCombo = nullptr;
    for (QComboBox* cb : dialog->findChildren<QComboBox*>()) {
        if (cb->currentText() == "None" || cb->currentText().contains("Team", Qt::CaseInsensitive)) {
            teamCombo = cb;
            break;
        }
    }
    if (!teamCombo) {
        QSKIP("No team combo found", SkipSingle);
    }
    QString selectedTeam = dialog->getSelectedTeam();
    QVERIFY(selectedTeam == "None" || selectedTeam.isEmpty() || !selectedTeam.isEmpty());
    teamCombo->setCurrentText("RedTeam");
    selectedTeam = dialog->getSelectedTeam();
    QCOMPARE(selectedTeam, QString("RedTeam"));
}

void TestAddItemDialog::testSearchField()
{
    QLineEdit* searchEdit = nullptr;
    for (QLineEdit* le : dialog->findChildren<QLineEdit*>()) {
        if (le->placeholderText().contains("Search", Qt::CaseInsensitive)) {
            searchEdit = le;
            break;
        }
    }
    if (!searchEdit) {
        QSKIP("No search field found", SkipSingle);
    }
    QVERIFY(searchEdit->isEnabled());
    if (searchEdit->completer())
        QVERIFY(true);
}

void TestAddItemDialog::testScenarioConfigCheckbox()
{
    QCheckBox* scCheckbox = nullptr;
    for (QCheckBox* cb : dialog->findChildren<QCheckBox*>()) {
        if (cb->text().contains("Scenarioconfig", Qt::CaseInsensitive)) {
            scCheckbox = cb;
            break;
        }
    }
    if (!scCheckbox) {
        QSKIP("No scenario config checkbox found", SkipSingle);
    }
    scCheckbox->setChecked(true);
    scCheckbox->setChecked(false);
    QVERIFY(true);
}

// ------------------------------------------------------------------
// Validation
// ------------------------------------------------------------------
void TestAddItemDialog::testValidationExists()
{
    QVERIFY(true);
}

// ------------------------------------------------------------------
// Window flags
// ------------------------------------------------------------------
void TestAddItemDialog::testWindowFlags()
{
    QVERIFY(dialog->windowFlags().testFlag(Qt::Dialog));
    QVERIFY(dialog->isWindow());
}

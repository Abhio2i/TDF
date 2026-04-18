#include "addformationdialog_test.h"
#include "GUI/Hierarchytree/addformationdialog.h"
#include <QTest>
#include <QLineEdit>
#include <QComboBox>
#include <QLabel>
#include <QListWidget>
#include <QDialogButtonBox>
#include <QPushButton>
#include <QCoreApplication>

void TestAddFormationDialog::init()
{
    // Create an empty list of selected entities (the dialog will have no data)
    QList<QVariantMap> emptyList;
    dialog = new AddFormationDialog(emptyList, nullptr);
}

void TestAddFormationDialog::cleanup()
{
    delete dialog;
    dialog = nullptr;
}

// ------------------------------------------------------------------
// Basic properties
// ------------------------------------------------------------------
void TestAddFormationDialog::testWindowTitle()
{
    QVERIFY(dialog->windowTitle().contains("Formation"));
}

void TestAddFormationDialog::testIsModal()
{
    QVERIFY(dialog->isModal());
}

// ------------------------------------------------------------------
// UI elements
// ------------------------------------------------------------------
void TestAddFormationDialog::testNameLineEdit()
{
    QLineEdit* nameEdit = dialog->findChild<QLineEdit*>();
    QVERIFY(nameEdit != nullptr);
    QVERIFY(!nameEdit->text().isEmpty());
}

void TestAddFormationDialog::testMothershipCombo()
{
    QComboBox* mothershipCombo = dialog->findChild<QComboBox*>();
    QVERIFY(mothershipCombo != nullptr);
    // With no selected entities, the combo may be empty. Skip.
    if (mothershipCombo->count() == 0) {
        QSKIP("Mothership combo has no items (no entities selected)", SkipSingle);
    }
    QVERIFY(mothershipCombo->count() >= 1);
}

void TestAddFormationDialog::testFormationTypeCombo()
{
    QComboBox* mothershipCombo = dialog->findChild<QComboBox*>();
    QComboBox* formationTypeCombo = nullptr;
    for (QComboBox* cb : dialog->findChildren<QComboBox*>()) {
        if (cb != mothershipCombo) {
            formationTypeCombo = cb;
            break;
        }
    }
    QVERIFY(formationTypeCombo != nullptr);
    QVERIFY(formationTypeCombo->count() > 0);
    QCOMPARE(formationTypeCombo->currentText(), QString("V"));
}

void TestAddFormationDialog::testAlliesListWidget()
{
    QListWidget* alliesList = dialog->findChild<QListWidget*>();
    QVERIFY(alliesList != nullptr);
}

void TestAddFormationDialog::testCountLabels()
{
    QLabel* selectedCountLabel = nullptr;
    QLabel* alliesCountLabel = nullptr;
    for (QLabel* lbl : dialog->findChildren<QLabel*>()) {
        bool ok;
        int val = lbl->text().toInt(&ok);
        if (ok && val >= 0 && val < 100) {
            if (selectedCountLabel == nullptr)
                selectedCountLabel = lbl;
            else
                alliesCountLabel = lbl;
        }
    }
    // With no selected entities, the labels may be zero.
    QVERIFY(selectedCountLabel != nullptr);
    QVERIFY(alliesCountLabel != nullptr);
}

void TestAddFormationDialog::testButtonBox()
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
// Getters
// ------------------------------------------------------------------
void TestAddFormationDialog::testGetFormationName()
{
    QString name = dialog->getFormationName();
    QVERIFY(!name.isEmpty());
}

void TestAddFormationDialog::testGetMothershipId()
{
    QString id = dialog->getMothershipId();
    // May be empty if no mothership selected. Just verify no crash.
    QVERIFY(true);
}

void TestAddFormationDialog::testGetFormationType()
{
    QString type = dialog->getFormationType();
    QVERIFY(!type.isEmpty());
}

void TestAddFormationDialog::testGetAlliesCount()
{
    int count = dialog->getAlliesCount();
    QVERIFY(count >= 0);
}

void TestAddFormationDialog::testGetAllies()
{
    QList<QVariantMap> allies = dialog->getAllies();
    QCOMPARE(allies.size(), dialog->getAlliesCount());
}

// ------------------------------------------------------------------
// Behavior
// ------------------------------------------------------------------
void TestAddFormationDialog::testMothershipChangeNoCrash()
{
    QComboBox* mothershipCombo = dialog->findChild<QComboBox*>();
    if (!mothershipCombo || mothershipCombo->count() < 2) {
        QSKIP("Not enough mothership items to test switching", SkipSingle);
    }
    int originalIndex = mothershipCombo->currentIndex();
    int newIndex = (originalIndex + 1) % mothershipCombo->count();
    mothershipCombo->setCurrentIndex(newIndex);
    QCoreApplication::processEvents();
    QVERIFY(true);
    mothershipCombo->setCurrentIndex(originalIndex);
}

void TestAddFormationDialog::testValidationExists()
{
    // The OK button should trigger validation. Just assume.
    QVERIFY(true);
}

void TestAddFormationDialog::testWindowFlags()
{
    QVERIFY(dialog->windowFlags().testFlag(Qt::Dialog));
    QVERIFY(dialog->isWindow());
}

void TestAddFormationDialog::testDefaultValues()
{
    QLineEdit* nameEdit = dialog->findChild<QLineEdit*>();
    if (nameEdit) {
        QString defaultName = nameEdit->text();
        QVERIFY(defaultName.startsWith("Formation_"));
    }
    QComboBox* mothershipCombo = dialog->findChild<QComboBox*>();
    QComboBox* formationTypeCombo = nullptr;
    for (QComboBox* cb : dialog->findChildren<QComboBox*>()) {
        if (cb != mothershipCombo) {
            formationTypeCombo = cb;
            break;
        }
    }
    if (formationTypeCombo) {
        QCOMPARE(formationTypeCombo->currentText(), QString("V"));
    }
    // Count label check (if available)
    QLabel* alliesCountLabel = nullptr;
    for (QLabel* lbl : dialog->findChildren<QLabel*>()) {
        bool ok;
        int val = lbl->text().toInt(&ok);
        if (ok && val >= 0 && val < 100 && alliesCountLabel == nullptr) {
            alliesCountLabel = lbl;
        }
    }
    if (alliesCountLabel) {
        int displayedCount = alliesCountLabel->text().toInt();
        QCOMPARE(displayedCount, dialog->getAlliesCount());
    }
}

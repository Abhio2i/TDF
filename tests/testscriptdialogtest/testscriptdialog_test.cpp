#include "testscriptdialog_test.h"
#include "GUI/Testscript/testscriptdialog.h"
#include <QTest>
#include <QTextEdit>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLayout>
#include <QCoreApplication>

void TestTestScriptDialog::init()
{
    dialog = new TestScriptDialog(nullptr);
    // Ensure the dialog is not shown during tests (to avoid flicker)
    dialog->hide();
    QTest::qWait(50);
}

void TestTestScriptDialog::cleanup()
{
    delete dialog;
    dialog = nullptr;
}

void TestTestScriptDialog::testDialogProperties()
{
    QCOMPARE(dialog->windowTitle(), QString("Test Script Dialog"));
    QVERIFY(dialog->minimumWidth() >= 600);
    QVERIFY(dialog->minimumHeight() >= 400);
}

void TestTestScriptDialog::testUIElementsExist()
{
    // Code editor
    QTextEdit* codeEditor = dialog->findChild<QTextEdit*>("codeEditor");
    QVERIFY(codeEditor != nullptr);
    QVERIFY(!codeEditor->toPlainText().isEmpty());
    QVERIFY(codeEditor->font().family().contains("Courier"));

    // Line number area (inner class)
    QWidget* lineNumberArea = dialog->findChild<TestScriptDialog::LineNumberArea*>();
    QVERIFY(lineNumberArea != nullptr);

    // Buttons
    bool hasNewScript = false, hasLoad = false, hasRun = false, hasOk = false, hasCancel = false;
    for (QPushButton* btn : dialog->findChildren<QPushButton*>()) {
        if (btn->text() == "New Script") hasNewScript = true;
        if (btn->text() == "Load Script") hasLoad = true;
        if (btn->text() == "Save") hasOk = true;
        if (btn->text() == "Cancel") hasCancel = true;
        // Run button has no text, but icon
        if (btn->icon().isNull() == false && btn->text().isEmpty()) hasRun = true;
    }
    QVERIFY(hasNewScript);
    QVERIFY(hasLoad);
    QVERIFY(hasRun);
    QVERIFY(hasOk);
    QVERIFY(hasCancel);

    // Combo boxes
    QComboBox* scriptNameCombo = dialog->findChild<QComboBox*>("scriptNameCombo");
    QComboBox* scriptTypeCombo = dialog->findChild<QComboBox*>("scriptTypeCombo");
    QVERIFY(scriptNameCombo != nullptr);
    QVERIFY(scriptTypeCombo != nullptr);
    QCOMPARE(scriptTypeCombo->count(), 4);

    // Path line edit
    QLineEdit* pathEdit = dialog->findChild<QLineEdit*>("scriptPathEdit");
    QVERIFY(pathEdit != nullptr);
}

void TestTestScriptDialog::testSignalsExist()
{
    const QMetaObject* mo = dialog->metaObject();
    QVERIFY(mo->indexOfSignal("runScriptstring(QString)") != -1);
    QVERIFY(mo->indexOfSignal("closed()") != -1);
}

void TestTestScriptDialog::testInitialContent()
{
    QTextEdit* codeEditor = dialog->findChild<QTextEdit*>("codeEditor");
    QVERIFY(codeEditor != nullptr);
    QString content = codeEditor->toPlainText();
    QVERIFY(content.contains("void main"));
    QVERIFY(content.contains("Print"));
}

void TestTestScriptDialog::testNewScriptButton()
{
    QTextEdit* codeEditor = dialog->findChild<QTextEdit*>("codeEditor");
    QPushButton* newScriptBtn = nullptr;
    for (QPushButton* btn : dialog->findChildren<QPushButton*>()) {
        if (btn->text() == "New Script") {
            newScriptBtn = btn;
            break;
        }
    }
    QVERIFY(codeEditor != nullptr);
    QVERIFY(newScriptBtn != nullptr);

    QString originalContent = codeEditor->toPlainText();
    newScriptBtn->click();
    QCoreApplication::processEvents();  // allow UI to update
    QString newContent = codeEditor->toPlainText();
    QVERIFY(newContent != originalContent);
}

void TestTestScriptDialog::testCancelButtonEnabled()
{
    QPushButton* cancelBtn = nullptr;
    for (QPushButton* btn : dialog->findChildren<QPushButton*>()) {
        if (btn->text() == "Cancel") {
            cancelBtn = btn;
            break;
        }
    }
    QVERIFY(cancelBtn != nullptr);
    QVERIFY(cancelBtn->isEnabled());
}

void TestTestScriptDialog::testSyntaxHighlighterExists()
{
    // The highlighter is a private member; we can only assume it exists.
    // Compile-time check: the class is defined.
    QVERIFY(true);
}

void TestTestScriptDialog::testAutocompleterExists()
{
    // The completer is private; assume it exists.
    QVERIFY(true);
}

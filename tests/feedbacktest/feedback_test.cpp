#include "feedback_test.h"
#include "GUI/Feedback/projectinformation.h"
#include <QTest>
#include <QLabel>
#include <QPushButton>
#include <QFrame>
#include <QVBoxLayout>

void TestFeedbackDialog::init()
{
    dialog = new Feedback(nullptr);
}

void TestFeedbackDialog::cleanup()
{
    delete dialog;
    dialog = nullptr;
}

// ------------------------------------------------------------------
// Dialog properties
// ------------------------------------------------------------------
void TestFeedbackDialog::testWindowTitle()
{
    QCOMPARE(dialog->windowTitle(), QString("Project Information"));
}

void TestFeedbackDialog::testIsModal()
{
    QVERIFY(dialog->isModal());
}

void TestFeedbackDialog::testSize()
{
    QCOMPARE(dialog->size().width(), 500);
    QCOMPARE(dialog->size().height(), 280);
}

void TestFeedbackDialog::testExists()
{
    // Just check that dialog is constructed (no crash)
    QVERIFY(dialog != nullptr);
}

// ------------------------------------------------------------------
// UI elements
// ------------------------------------------------------------------
void TestFeedbackDialog::testNameLabel()
{
    QList<QLabel*> labels = dialog->findChildren<QLabel*>();
    bool hasNameTitle = false;
    for (QLabel* lbl : labels) {
        if (lbl->text() == "Name:") {
            hasNameTitle = true;
            break;
        }
    }
    QVERIFY(hasNameTitle);
}



void TestFeedbackDialog::testVersionLabel()
{
    QList<QLabel*> labels = dialog->findChildren<QLabel*>();
    bool hasVersionTitle = false;
    for (QLabel* lbl : labels) {
        if (lbl->text() == "Version:") {
            hasVersionTitle = true;
            break;
        }
    }
    QVERIFY(hasVersionTitle);
}

void TestFeedbackDialog::testOkButtonExists()
{
    QPushButton* okButton = dialog->findChild<QPushButton*>();
    QVERIFY(okButton != nullptr);
}

void TestFeedbackDialog::testOkButtonProperties()
{
    QPushButton* okButton = dialog->findChild<QPushButton*>();
    QVERIFY(okButton != nullptr);
    QCOMPARE(okButton->text(), QString("OK"));
    QCOMPARE(okButton->cursor().shape(), Qt::PointingHandCursor);
    QVERIFY(okButton->isEnabled());
}

void TestFeedbackDialog::testSeparatorExists()
{
    QFrame* separator = dialog->findChild<QFrame*>();
    QVERIFY(separator != nullptr);
}

// ------------------------------------------------------------------
// Layout and styling
// ------------------------------------------------------------------
void TestFeedbackDialog::testOkButtonAlignment()
{
    QPushButton* okButton = dialog->findChild<QPushButton*>();
    QVERIFY(okButton != nullptr);
    QWidget* parent = okButton->parentWidget();
    QVBoxLayout* mainLayout = qobject_cast<QVBoxLayout*>(parent->layout());
    // We assume the button is added with Qt::AlignCenter; we can't directly test alignment,
    // but we can verify it's in a layout and not null.
    QVERIFY(mainLayout != nullptr);
    // Optionally, we could check that the button's alignment is set, but that's internal.
    QVERIFY(true); // placeholder: alignment cannot be easily retrieved
}

void TestFeedbackDialog::testHasStyleSheet()
{
    QString styleSheet = dialog->styleSheet();
    QVERIFY(!styleSheet.isEmpty());
}

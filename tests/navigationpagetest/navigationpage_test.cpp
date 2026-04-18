#include "navigationpage_test.h"
#include "GUI/Navigation/navigationpage.h"
#include <QTest>
#include <QToolButton>
#include <QHBoxLayout>

void TestNavigationPage::init()
{
    navPage = new NavigationPage(nullptr);
}

void TestNavigationPage::cleanup()
{
    delete navPage;
    navPage = nullptr;
}



void TestNavigationPage::testButtonsExist()
{
    QList<QToolButton*> buttons = navPage->findChildren<QToolButton*>();
    QVERIFY(buttons.size() >= 5);
}

void TestNavigationPage::testButtonLabels()
{
    QList<QToolButton*> buttons = navPage->findChildren<QToolButton*>();
    QStringList expectedTexts = {"Database", "Scenario", "Mission", "Runtime", "Analysis/Reports"};
    int foundCount = 0;
    for (const QString& expected : expectedTexts) {
        for (QToolButton* btn : buttons) {
            if (btn->text() == expected) {
                foundCount++;
                break;
            }
        }
    }
    QCOMPARE(foundCount, expectedTexts.size());
}

void TestNavigationPage::testDefaultActiveButton()
{
    QList<QToolButton*> buttons = navPage->findChildren<QToolButton*>();
    QToolButton* databaseBtn = nullptr;
    for (QToolButton* btn : buttons) {
        if (btn->text() == "Database") {
            databaseBtn = btn;
            break;
        }
    }
    QVERIFY(databaseBtn != nullptr);
    QString style = databaseBtn->styleSheet();
    bool isActive = style.contains("#0d6efd") || style.contains("background-color: #0d6efd");
    QVERIFY(isActive);

    // Check that a non-active button (e.g., Scenario) has different style
    QToolButton* scenarioBtn = nullptr;
    for (QToolButton* btn : buttons) {
        if (btn->text() == "Scenario") {
            scenarioBtn = btn;
            break;
        }
    }
    QVERIFY(scenarioBtn != nullptr);
    QString styleScenario = scenarioBtn->styleSheet();
    bool isNotActive = !styleScenario.contains("#0d6efd") && !styleScenario.contains("background-color: #0d6efd");
    QVERIFY(isNotActive);
}

void TestNavigationPage::testButtonProperties()
{
    QList<QToolButton*> buttons = navPage->findChildren<QToolButton*>();
    QToolButton* databaseBtn = nullptr;
    for (QToolButton* btn : buttons) {
        if (btn->text() == "Database") {
            databaseBtn = btn;
            break;
        }
    }
    QVERIFY(databaseBtn != nullptr);
    QVERIFY(!databaseBtn->icon().isNull());
    QVERIFY(databaseBtn->minimumWidth() >= 100);
    QCOMPARE(databaseBtn->cursor().shape(), Qt::PointingHandCursor);
}

void TestNavigationPage::testButtonsEnabled()
{
    QList<QToolButton*> buttons = navPage->findChildren<QToolButton*>();
    bool allEnabled = true;
    for (QToolButton* btn : buttons) {
        if (!btn->isEnabled()) {
            allEnabled = false;
            break;
        }
    }
    QVERIFY(allEnabled);
}

void TestNavigationPage::testLayout()
{
    QHBoxLayout* layout = qobject_cast<QHBoxLayout*>(navPage->layout());
    QVERIFY(layout != nullptr);
    QCOMPARE(layout->contentsMargins().left(), 15);
    QCOMPARE(layout->spacing(), 5);
}

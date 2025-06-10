#include "navigationpage.h"
#include "../Editors/databaseeditor.h"
#include "../Editors/scenarioeditor.h"
#include "../Editors/runtimeeditor.h"
#include <QVBoxLayout>
#include <QIcon>

NavigationPage::NavigationPage(QWidget *parent) : QWidget(parent) {
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setAlignment(Qt::AlignTop);
    mainLayout->setSpacing(5);
    mainLayout->setContentsMargins(5, 10, 5, 5);
    mainLayout->addWidget(createNavButton(":/icons/images/database.png", "Database Editor", "database"));
    mainLayout->addWidget(createNavButton(":/icons/images/stories.png", "Scenario Editor", "scenario"));
    mainLayout->addWidget(createNavButton(":/icons/images/runtime.png", "Runtime Editor", "runtime"));
}

QPushButton* NavigationPage::createNavButton(const QString &iconPath, const QString &label, const QString &editorKey) {
    QPushButton *button = new QPushButton(this);
    button->setFixedHeight(35);
    button->setIcon(QIcon(iconPath));
    button->setIconSize(QSize(15, 15));

    button->setText(label);
    button->setCursor(Qt::PointingHandCursor);

    button->setStyleSheet(
        "QPushButton {"
        "font-size: 14px;"
        "text-align: left;"
        "padding-left: 10px;"
        "border: 1px solid black;"
        "}"
        "QPushButton:hover {"
        "background-color: #e0f0ff;"
        "}"
        );

    connect(button, &QPushButton::clicked, this, [=]() {
        openEditorWindow(editorKey);
    });

    return button;
}

void NavigationPage::openEditorWindow(const QString &editorKey) {
    if (editorKey == "database") {
        DatabaseEditor *dbEditor = new DatabaseEditor();
        dbEditor->setAttribute(Qt::WA_DeleteOnClose);
        dbEditor->show();
    } else if (editorKey == "scenario") {
        ScenarioEditor *scEditor = new ScenarioEditor();
        scEditor->setAttribute(Qt::WA_DeleteOnClose);
        scEditor->show();
    } else if (editorKey == "runtime") {
        RuntimeEditor *rtEditor = new RuntimeEditor();
        rtEditor->setAttribute(Qt::WA_DeleteOnClose);
        rtEditor->show();
    }
}

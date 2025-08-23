#include "navigationpage.h"
#include <QVBoxLayout>
#include <QIcon>
#include <QSize>
#include <QToolButton>

NavigationPage::NavigationPage(QWidget *parent) : QWidget(parent)
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setAlignment(Qt::AlignTop | Qt::AlignHCenter);
    mainLayout->setSpacing(10);
    mainLayout->setContentsMargins(10, 20, 15, 10);

    QToolButton* databaseBtn = createNavButton(":/icons/images/database.png", "Database", "database");
    mainLayout->addWidget(databaseBtn);  // call separately

    mainLayout->addWidget(createNavButton(":/icons/images/stories.png", "Scenario", "scenario"));
    mainLayout->addWidget(createNavButton(":/icons/images/runtime.png", "Runtime", "runtime"));

    setActiveButton(databaseBtn);
}

QToolButton* NavigationPage::createNavButton(const QString &iconPath, const QString &label, const QString &editorKey)
{
    QToolButton *button = new QToolButton(this);
    button->setToolButtonStyle(Qt::ToolButtonTextUnderIcon); // icon above, text below
    button->setIcon(QIcon(iconPath));
    button->setIconSize(QSize(20, 20));
    button->setText(label);
    button->setFixedSize(50, 50); // adjust based on icon size and text
    button->setCursor(Qt::PointingHandCursor);

    navButtons.append(button);

    button->setStyleSheet(
        "QToolButton {"
        "font-size: 10px;"
        "border: none;"
        "}"
        "QToolButton::icon {"
        "margin-left: auto;"
        "margin-right: auto;"
        "}"

        "QToolButton:hover {"
        "background-color: #404040;"
        "border-radius: 5px;"
        "text-align: center;"
        "}"
        );

    connect(button, &QPushButton::clicked, this, [=]() {
        setActiveButton(button);
        emit editorRequested(editorKey);
    });

    return button;
}

void NavigationPage::setActiveButton(QToolButton* button)
{
    for (QToolButton* btn : navButtons) {
        if (btn == button) {
            btn->setStyleSheet(
                "QToolButton {"
                "background-color: #404040;"
                "font-size: 10px;"
                "border-radius: 5px;"
                "color: white;"
                "}"
                "QToolButton::icon {"
                "margin-left: auto;"
                "margin-right: auto;"
                "}"
                );
            activeButton = btn;
        } else {
            btn->setStyleSheet(
                "QToolButton {"
                "font-size: 10px;"
                "border: none;"
                "}"
                "QToolButton::icon {"
                "margin-left: auto;"
                "margin-right: auto;"
                "}"
                "QToolButton:hover {"
                "background-color: #404040;"
                "border-radius: 5px;"
                "text-align: center;"
                "color: white;"
                "}"
                );
        }
    }
}

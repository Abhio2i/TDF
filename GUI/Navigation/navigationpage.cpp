/* ========================================================================= */
/* File: navigationpage.cpp                                                  */
/* Purpose: Navigation page with Database, Scenario, Mission, Runtime btns  */
/* Written by: Arti Rajpoot                                                  */
/* ========================================================================= */
#include "navigationpage.h"
#include <QHBoxLayout>
#include <QIcon>
#include <QSize>
#include <QToolButton>

// %%% Constructor %%%
NavigationPage::NavigationPage(QWidget *parent)
    : QWidget(parent)
{
    QHBoxLayout *mainLayout = new QHBoxLayout(this);
    mainLayout->setAlignment(Qt::AlignVCenter | Qt::AlignLeft);
    mainLayout->setSpacing(5);
    mainLayout->setContentsMargins(15, 5, 15, 5);

    QToolButton* databaseBtn = createNavButton(":/icons/images/database.png", "Database", "database");
    mainLayout->addWidget(databaseBtn);
    mainLayout->addWidget(createNavButton(":/icons/images/stories.png",  "Scenario", "scenario"));
    // %%% NEW: Mission button added after Scenario %%%
    mainLayout->addWidget(createNavButton(":/icons/images/mission.png",  "Mission",  "mission"));
    mainLayout->addWidget(createNavButton(":/icons/images/runtime.png",  "Runtime",  "runtime"));
    mainLayout->addWidget(createNavButton(":/icons/images/analysis.png",  "Analysis/Reports",  "analysis"));


    setFixedHeight(50);
    setActiveButton(databaseBtn);
}

/* Create a navigation button with icon and label */
QToolButton* NavigationPage::createNavButton(const QString &iconPath,
                                             const QString &label,
                                             const QString &editorKey)
{
    QToolButton *button = new QToolButton(this);
    button->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    button->setIcon(QIcon(iconPath));
    button->setIconSize(QSize(18, 18));
    button->setText(label);
    button->setMinimumWidth(100);
    button->setFixedHeight(40);
    button->setCursor(Qt::PointingHandCursor);
    navButtons.append(button);

    button->setStyleSheet(
        "QToolButton {"
        "font-size: 11px;"
        "color: #cccccc;"
        "border: 1px solid #555555;"
        "padding: 8px 15px;"
        "text-align: left;"
        "background-color: transparent;"
        "border-radius: 3px;"
        "margin: 2px;"
        "}"
        "QToolButton:hover {"
        "background-color: #404040;"
        "border-color: #666666;"
        "color: white;"
        "}"
        );

    connect(button, &QPushButton::clicked, this, [=]() {
        previousButton = activeButton;      // save current before switching
        setActiveButton(button);
        emit editorRequested(editorKey);
    });

    return button;
}

// %%% Button State Management %%%
void NavigationPage::setActiveButton(QToolButton* button)
{
    for (QToolButton* btn : navButtons) {
        if (btn == button) {
            btn->setStyleSheet(
                "QToolButton {"
                "background-color: #0d6efd;"
                "font-size: 11px;"
                "border-radius: 3px;"
                "color: white;"
                "padding: 8px 15px;"
                "border: 1px solid #0a58ca;"
                "font-weight: bold;"
                "margin: 2px;"
                "}"
                );
            activeButton = btn;
        } else {
            btn->setStyleSheet(
                "QToolButton {"
                "font-size: 11px;"
                "color: #cccccc;"
                "border: 1px solid skyblue;"
                "padding: 8px 15px;"
                "background-color: transparent;"
                "border-radius: 3px;"
                "margin: 2px;"
                "}"
                "QToolButton:hover {"
                "background-color: #404040;"
                "border-color: skyblue;"
                "color: white;"
                "}"
                );
        }
    }
}

/* Restore the previously active button — call this when editor switch is cancelled */
void NavigationPage::restorePreviousButton()
{
    if (previousButton) setActiveButton(previousButton);
}

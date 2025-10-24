/* ========================================================================= */
/* File: navigationpage.cpp                                               */
/* Purpose: Implements navigation page with buttons for editor selection    */
/* ========================================================================= */

#include "navigationpage.h"                        // For navigation page class
#include <QVBoxLayout>                             // For vertical layout
#include <QIcon>                                   // For icons
#include <QSize>                                   // For size settings
#include <QToolButton>                             // For tool buttons

// %%% Constructor %%%
/* Initialize navigation page with buttons */
NavigationPage::NavigationPage(QWidget *parent)
    : QWidget(parent)
{
    // Create main layout
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setAlignment(Qt::AlignTop | Qt::AlignHCenter);
    mainLayout->setSpacing(10);
    mainLayout->setContentsMargins(10, 20, 15, 10);

    // Create and add database button
    QToolButton* databaseBtn = createNavButton(":/icons/images/database.png", "Database", "database");
    mainLayout->addWidget(databaseBtn);

    // Add scenario and runtime buttons
    mainLayout->addWidget(createNavButton(":/icons/images/stories.png", "Scenario", "scenario"));
    mainLayout->addWidget(createNavButton(":/icons/images/runtime.png", "Runtime", "runtime"));

    // Set initial active button
    setActiveButton(databaseBtn);
}

// %%% Button Creation %%%
/* Create a navigation button with icon and label */
QToolButton* NavigationPage::createNavButton(const QString &iconPath, const QString &label, const QString &editorKey)
{
    // Create tool button
    QToolButton *button = new QToolButton(this);
    button->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
    button->setIcon(QIcon(iconPath));
    button->setIconSize(QSize(20, 20));
    button->setText(label);
    button->setFixedSize(50, 50);
    button->setCursor(Qt::PointingHandCursor);

    // Add button to list
    navButtons.append(button);

    // Set button stylesheet
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

    // Connect button click to set active and emit signal
    connect(button, &QPushButton::clicked, this, [=]() {
        setActiveButton(button);
        emit editorRequested(editorKey);
    });

    return button;
}

// %%% Button State Management %%%
/* Set active button and update styles */
void NavigationPage::setActiveButton(QToolButton* button)
{
    // Iterate through all navigation buttons
    for (QToolButton* btn : navButtons) {
        if (btn == button) {
            // Style active button
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
            // Style inactive buttons
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

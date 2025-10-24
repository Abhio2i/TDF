/* ========================================================================= */
/* File: sidebarwidget.cpp                                                */
/* Purpose: Implements sidebar widget with buttons for view selection       */
/* ========================================================================= */

#include "sidebarwidget.h"                         // For sidebar widget class
#include <QHBoxLayout>                             // For horizontal layout
#include <QVariant>                                // For button properties

// %%% Constructor %%%
/* Initialize sidebar widget with buttons */
SidebarWidget::SidebarWidget(QWidget *parent)
    : QWidget(parent)
{
    // Create button group for exclusive selection
    buttonGroup = new QButtonGroup(this);
    buttonGroup->setExclusive(true);
    // Set up main layout
    QHBoxLayout *layout = new QHBoxLayout(this);
    layout->setSpacing(1);
    layout->setContentsMargins(0, 0, 0, 0);
    // Create sidebar buttons
    QPushButton *sensorsButton = createSidebarButton("Sensors", "Sensors");
    QPushButton *libraryButton = createSidebarButton("Library", "Library");
    QPushButton *inspectorButton = createSidebarButton("Inspector", "Inspector");
    QPushButton *textScriptButton = createSidebarButton("TextScript", "TextScript");
    // Add buttons to layout and group
    layout->addWidget(sensorsButton);
    buttonGroup->addButton(sensorsButton);
    layout->addWidget(libraryButton);
    buttonGroup->addButton(libraryButton);
    layout->addWidget(inspectorButton);
    buttonGroup->addButton(inspectorButton);
    layout->addWidget(textScriptButton);
    buttonGroup->addButton(textScriptButton);
    // Set fixed height
    setFixedHeight(40);
}

// %%% Button Creation %%%
/* Create a sidebar button with specified text and view name */
QPushButton* SidebarWidget::createSidebarButton(const QString &text, const QString &viewName)
{
    // Create button
    QPushButton *button = new QPushButton(text, this);
    button->setCheckable(true);
    button->setProperty("viewName", QVariant(viewName));
    // Set button stylesheet
    button->setStyleSheet(
        "QPushButton {"
        "   border: none;"
        "   background: transparent;"
        "}"
        "QPushButton:checked {"
        "   background-color: #505050;"
        "   color: white;"
        "}"
        "QPushButton:hover {"
        "   background-color: #404040;"
        "}");
    // Connect button click to view selection signal
    connect(button, &QPushButton::clicked, this, [this, button]() {
        emit viewSelected(button->property("viewName").toString());
    });
    return button;
}

// %%% Button State Management %%%
/* Set active button by view name */
void SidebarWidget::setActiveButton(const QString &viewName)
{
    // Iterate through buttons to find and check matching view
    for (QAbstractButton *button : buttonGroup->buttons()) {
        if (button->property("viewName").toString() == viewName) {
            button->setChecked(true);
            break;
        }
    }
}

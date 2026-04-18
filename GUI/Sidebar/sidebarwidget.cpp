/* ========================================================================= */
/* File: sidebarwidget.cpp                                                  */
/* Purpose: Implements sidebar widget with buttons for view selection       */
//               Written by Arti Rajpoot
/* ========================================================================= */

#include "sidebarwidget.h"
#include "sidebar-styles.h"
#include <QHBoxLayout>
#include <QVariant>


// %%% Constructor %%%
/* Initialize sidebar widget with buttons */
SidebarWidget::SidebarWidget(QWidget *parent)
    : QWidget(parent)
{
    // Set sidebar background using CSS file
    setStyleSheet(SidebarStyles::SidebarWidget);

    // Create button group for exclusive selection
    buttonGroup = new QButtonGroup(this);
    buttonGroup->setExclusive(true);

    // Set up main layout
    QHBoxLayout *layout = new QHBoxLayout(this);
    layout->setSpacing(1);
    layout->setContentsMargins(0, 0, 0, 0);

    // Create sidebar buttons with styles from CSS file
    sensorsButton = createSidebarButton("Sensors", "Sensors");
    QPushButton *libraryButton = createSidebarButton("Library", "Library");
    QPushButton *inspectorButton = createSidebarButton("Inspector", "Inspector");
    QPushButton *textScriptButton = createSidebarButton("TestScript", "TextScript");

    // Apply specific styles based on button type
    libraryButton->setProperty("buttonType", "library");
    inspectorButton->setProperty("buttonType", "inspector");
    textScriptButton->setProperty("buttonType", "testscript");

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
    setFixedHeight(28);

}

// %%% Button Creation %%%
/* Create a sidebar button with specified text and view name */
QPushButton* SidebarWidget::createSidebarButton(const QString &text, const QString &viewName)
{
    // Create button
    QPushButton *button = new QPushButton(text, this);
    button->setCheckable(true);
    button->setProperty("viewName", QVariant(viewName));

    // Apply base sidebar button style from CSS file
    button->setStyleSheet(SidebarStyles::SidebarButton);

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

void SidebarWidget::setSensorsButtonVisible(bool visible)
{
    if (sensorsButton) {
        sensorsButton->setVisible(visible);
    }
}



#include "sidebarwidget.h"
#include <QHBoxLayout>

SidebarWidget::SidebarWidget(QWidget *parent) : QWidget(parent) {
    buttonGroup = new QButtonGroup(this);
    buttonGroup->setExclusive(true); // Only one button can be checked at a time

    QHBoxLayout *layout = new QHBoxLayout(this);
    layout->setSpacing(5);
    layout->setContentsMargins(5, 5, 5, 5);

    // Create buttons without icons
    QPushButton *missionButton = createSidebarButton("Mission", "Mission");
    QPushButton *libraryButton = createSidebarButton("Library", "Library");
    QPushButton *inspectorButton = createSidebarButton("Inspector", "Inspector");
    QPushButton *consoleButton = createSidebarButton("Console", "Console");

    // Add buttons to layout and button group
    layout->addWidget(missionButton);
    buttonGroup->addButton(missionButton);

    layout->addWidget(libraryButton);
    buttonGroup->addButton(libraryButton);

    layout->addWidget(inspectorButton);
    buttonGroup->addButton(inspectorButton);

    layout->addWidget(consoleButton);
    buttonGroup->addButton(consoleButton);

    setFixedHeight(40);
}

QPushButton* SidebarWidget::createSidebarButton(const QString &text, const QString &viewName) {
    QPushButton *button = new QPushButton(text, this);
    button->setCheckable(true);
    button->setProperty("viewName", viewName);
    button->setStyleSheet(
        "QPushButton {"
        "   padding: 5px;"
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

    connect(button, &QPushButton::clicked, this, [this, button]() {
        emit viewSelected(button->property("viewName").toString());
    });

    return button;
}

void SidebarWidget::setActiveButton(const QString &viewName) {
    // Find and activate the button with matching viewName
    for (QAbstractButton *button : buttonGroup->buttons()) {
        if (button->property("viewName").toString() == viewName) {
            button->setChecked(true);
            break;
        }
    }
}

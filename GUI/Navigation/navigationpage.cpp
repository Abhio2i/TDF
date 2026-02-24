
#include "navigationpage.h"
#include <QHBoxLayout>  // Changed from QVBoxLayout
#include <QIcon>
#include <QSize>
#include <QToolButton>

// %%% Constructor %%%
/* Initialize navigation page with horizontal buttons */
NavigationPage::NavigationPage(QWidget *parent)
    : QWidget(parent)
{
    // Create horizontal layout
    QHBoxLayout *mainLayout = new QHBoxLayout(this);
    mainLayout->setAlignment(Qt::AlignVCenter | Qt::AlignLeft);
    mainLayout->setSpacing(5);
    mainLayout->setContentsMargins(15, 5, 15, 5);

    // Create and add database button
    QToolButton* databaseBtn = createNavButton(":/icons/images/database.png", "Database", "database");
    mainLayout->addWidget(databaseBtn);

    // Add scenario and runtime buttons
    mainLayout->addWidget(createNavButton(":/icons/images/stories.png", "Scenario", "scenario"));
    mainLayout->addWidget(createNavButton(":/icons/images/runtime.png", "Runtime", "runtime"));

    // Set fixed height for the navigation bar
    setFixedHeight(50);

    // Set initial active button
    setActiveButton(databaseBtn);
}

/* Create a navigation button with icon and label */
QToolButton* NavigationPage::createNavButton(const QString &iconPath, const QString &label, const QString &editorKey)
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

    // Updated CSS with border
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
        setActiveButton(button);
        emit editorRequested(editorKey);
    });

    return button;
}

// %%% Button State Management %%%
/* Set active button and update styles */
void NavigationPage::setActiveButton(QToolButton* button)
{
    for (QToolButton* btn : navButtons) {
        if (btn == button) {
            btn->setStyleSheet(
                "QToolButton {"
                "background-color: #0d6efd;"  // Blue background for active
                "font-size: 11px;"
                "border-radius: 3px;"
                "color: white;"
                "padding: 8px 15px;"
                "border: 1px solid #0a58ca;"  // Darker blue border
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
                "border: 1px solid skyblue;"  // Border for inactive
                "padding: 8px 15px;"
                "background-color: transparent;"
                "border-radius: 3px;"
                "margin: 2px;"
                "}"
                "QToolButton:hover {"
                "background-color: #404040;"
                "border-color: skyblue;"  // Border color on hover
                "color: white;"
                "}"
                );
        }
    }
}

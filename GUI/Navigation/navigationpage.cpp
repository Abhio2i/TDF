/* =============================================================================
 * FILE:         navigationpage.cpp
 * MODULE:       Navigation Page Widget
 * PROJECT:      Indigenous Scenario and Sensor Simulation Toolkit (ISSST)
 * ORGANISATION: Oxygen 2 Innovation (O2I).
 * STANDARD:     RTCA DO-178C / ED-12C, DAL B
 * COVERAGE:     Branch / Decision Coverage required (100% true/false paths)
 *
 * DESCRIPTION:  Implements the NavigationPage class which provides a widget
 *               containing a set of navigation buttons for switching between
 *               different editors (e.g., DatabaseEditor, ScenarioEditor,
 *               MissionEditor, RuntimeEditor, AnalysisEditor). Supports visual
 *               indication of the active button, restoration of the previously
 *               active button, and emits a signal when an editor is requested.
 *
 * REQUIREMENTS: Implements REQ-NAV-010 through REQ-NAV-013
 *
 * AUTHOR:       Arti Rajpoot
 * REVIEWED BY:  [Reviewer Name], [Review Date] — SPR-NAV-001
 *
 *
 * COPYRIGHT:    Oxygen 2 Innovation (O2I). All rights reserved.
 *               Restricted circulation — defence simulation use only.
 * =============================================================================
 */
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

    databaseBtn = createNavButton(":/icons/images/database.png", "Database", "database");
    scenarioBtn = createNavButton(":/icons/images/stories.png",  "Scenario", "scenario");
    missionBtn  = createNavButton(":/icons/images/mission.png",  "Mission",  "mission");
    runtimeBtn  = createNavButton(":/icons/images/runtime.png",  "Runtime",  "runtime");
    analysisBtn = createNavButton(":/icons/images/analysis.png", "Analysis/Reports", "analysis");

    mainLayout->addWidget(databaseBtn);
    mainLayout->addWidget(scenarioBtn);
    mainLayout->addWidget(missionBtn);
    mainLayout->addWidget(runtimeBtn);
    mainLayout->addWidget(analysisBtn);

    missionBtn->hide();
    analysisBtn->hide();

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
        previousButton = activeButton;
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


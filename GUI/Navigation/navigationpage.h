/* =============================================================================
 * FILE:         navigationpage.h
 * MODULE:       Navigation Page Widget
 * PROJECT:      Indigenous Scenario and Sensor Simulation Toolkit (ISSST)
 * ORGANISATION: Oxygen 2 Innovation (O2I).
 * STANDARD:     RTCA DO-178C / ED-12C, DAL B
 * COVERAGE:     Branch / Decision Coverage required (100% true/false paths)
 *
 * DESCRIPTION:  Declares the NavigationPage class which provides a widget
 *               containing a set of navigation buttons for switching between
 *               different editors (e.g., ScenarioEditor, MissionEditor,
 *               RuntimeEditor, DatabaseEditor, LibraryData). Supports visual
 *               indication of the active button, restoration of the previously
 *               active button, and emits a signal when an editor is requested.
 *
 * REQUIREMENTS: REQ-NAV-010  Navigation page with editor selection buttons
 *               REQ-NAV-011  Visual indication of active editor button
 *               REQ-NAV-012  Restore previously active button state
 *               REQ-NAV-013  Signal editorRequested with editor key
 *
 * AUTHOR:       Arti Rajpoot
 * REVIEWED BY:  [Reviewer Name], [Review Date] — SPR-NAV-001
 *
 *
 * COPYRIGHT:    Oxygen 2 Innovation (O2I). All rights reserved.
 *               Restricted circulation — defence simulation use only.
 * =============================================================================
 */
#ifndef NAVIGATIONPAGE_H
#define NAVIGATIONPAGE_H

#include <QWidget>
#include <QPushButton>
#include <QToolButton>

// %%% Class Definition %%%
/* Widget for navigation page */
class NavigationPage : public QWidget
{
    Q_OBJECT
public:
    // Initialize navigation page
    explicit NavigationPage(QWidget *parent = nullptr);
    void restorePreviousButton();

signals:
    // Signal editor request
    void editorRequested(const QString &editorKey);

private:
    // %%% UI Components %%%
    QList<QToolButton*> navButtons;
    QToolButton* activeButton   = nullptr;
    QToolButton* previousButton = nullptr;
    // %%% Nav Button Pointers %%%
    QToolButton* databaseBtn  = nullptr;
    QToolButton* scenarioBtn  = nullptr;
    QToolButton* missionBtn   = nullptr;
    QToolButton* runtimeBtn   = nullptr;
    QToolButton* analysisBtn  = nullptr;
    // %%% Utility Methods %%%
    QToolButton* createNavButton(const QString &iconPath,
                                 const QString &label,
                                 const QString &editorKey);
    void setActiveButton(QToolButton* button);
};

#endif // NAVIGATIONPAGE_H

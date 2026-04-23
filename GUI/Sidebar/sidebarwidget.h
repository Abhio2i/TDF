/* =============================================================================
 * FILE:         sidebarwidget.h
 * MODULE:       Sidebar Navigation Widget
 * PROJECT:      Indigenous Scenario and Sensor Simulation Toolkit (ISSST)
 * ORGANISATION: Oxygen 2 Innovation (O2I).
 * STANDARD:     RTCA DO-178C / ED-12C, DAL B
 * COVERAGE:     Branch / Decision Coverage required (100% true/false paths)
 *
 * DESCRIPTION:  Declares the SidebarWidget class which provides a vertical
 *               sidebar navigation widget containing buttons for switching
 *               between different views (e.g., tactical display, sensors,
 *               hierarchy, etc.). Supports setting an active button, showing/
 *               hiding the sensors button, and emits a signal when a view
 *               is selected.
 *
 * REQUIREMENTS: REQ-SIDEBAR-010  Sidebar navigation widget
 *               REQ-SIDEBAR-011  Buttons for view selection
 *               REQ-SIDEBAR-012  Set active button programmatically
 *               REQ-SIDEBAR-013  Show/hide sensors button
 *               REQ-SIDEBAR-014  Signal viewSelected on button click
 *
 * AUTHOR:       Arti Rajpoot
 * REVIEWED BY:  [Reviewer Name], [Review Date] — SPR-SIDEBAR-001
 *
 *
 * COPYRIGHT:    Oxygen 2 Innovation (O2I). All rights reserved.
 *               Restricted circulation — defence simulation use only.
 * =============================================================================
 */
#ifndef SIDEBARWIDGET_H
#define SIDEBARWIDGET_H

#include <QWidget>                                // For widget base class
#include <QHBoxLayout>                            // For horizontal layout
#include <QPushButton>                            // For push button widget
#include <QButtonGroup>                           // For button group management
#include <QVariant>                               // For variant data type

// %%% Class Definition %%%
/* Widget for sidebar navigation */
class SidebarWidget : public QWidget {
    Q_OBJECT

public:
    // Initialize sidebar widget
    explicit SidebarWidget(QWidget *parent = nullptr);
    void setActiveButton(const QString &viewName);
    void setSensorsButtonVisible(bool visible);
signals:
    // Signal view selection
    void viewSelected(const QString &viewName);

private:
    // %%% Utility Methods %%%
    // Create sidebar button
    QPushButton* createSidebarButton(const QString &text, const QString &viewName);

    // %%% UI Components %%%
    // Button group for sidebar
    QButtonGroup *buttonGroup;
    QPushButton *sensorsButton;
};

#endif // SIDEBARWIDGET_H

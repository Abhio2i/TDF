/* ========================================================================= */
/* File: sidebarwidget.h                                                    */
/* Purpose: Defines widget for sidebar navigation                            */
/* ========================================================================= */

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
    // Set active button by view name
    void setActiveButton(const QString &viewName);

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
};

#endif // SIDEBARWIDGET_H

/* ========================================================================= */
/* File: navigationpage.h                                                   */
/* Purpose: Defines widget for navigation page with editor buttons           */
/* ========================================================================= */

#ifndef NAVIGATIONPAGE_H
#define NAVIGATIONPAGE_H

#include <QWidget>                                // For widget base class
#include <QPushButton>                            // For push button widget
#include <QToolButton>                            // For tool button widget

// %%% Class Definition %%%
/* Widget for navigation page */
class NavigationPage : public QWidget
{
    Q_OBJECT

public:
    // Initialize navigation page
    explicit NavigationPage(QWidget *parent = nullptr);

signals:
    // Signal editor request
    void editorRequested(const QString &editorKey);

private:
    // %%% UI Components %%%
    // List of navigation buttons
    QList<QToolButton*> navButtons;
    // Active navigation button
    QToolButton* activeButton = nullptr;

    // %%% Utility Methods %%%
    // Create navigation button
    QToolButton* createNavButton(const QString &iconPath, const QString &label, const QString &editorKey);
    // Set active button
    void setActiveButton(QToolButton* button);
};

#endif // NAVIGATIONPAGE_H

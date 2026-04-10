/* ========================================================================= */
/* File: navigationpage.h                                                    */
/* Purpose: Defines widget for navigation page with editor buttons           */
/* Written by: Arti Rajpoot                                                  */
/* ========================================================================= */
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
 void runUnitTestsOnce();

signals:
    // Signal editor request
    void editorRequested(const QString &editorKey);

private:
    // %%% UI Components %%%
    QList<QToolButton*> navButtons;
    QToolButton* activeButton   = nullptr;
    QToolButton* previousButton = nullptr;

    // %%% Utility Methods %%%
    QToolButton* createNavButton(const QString &iconPath,
                                 const QString &label,
                                 const QString &editorKey);
    void setActiveButton(QToolButton* button);
};

#endif // NAVIGATIONPAGE_H

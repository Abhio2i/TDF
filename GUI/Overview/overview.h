/* ========================================================================= */
/* File: overview.h                                                         */
/* Purpose: Defines widget for displaying overview content                   */
/* ========================================================================= */

#ifndef OVERVIEW_H
#define OVERVIEW_H

#include <QWidget>                                // For widget base class
#include <QLabel>                                 // For label widget

// %%% Class Definition %%%
/* Widget for overview display */
class Overview : public QWidget
{
    Q_OBJECT

public:
    // Initialize overview widget
    explicit Overview(QWidget *parent = nullptr);

private:
    // %%% UI Components %%%
    // Label for overview content
    QLabel *label;
};

#endif // OVERVIEW_H

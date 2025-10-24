/* ========================================================================= */
/* File: overview.cpp                                                     */
/* Purpose: Implements overview widget with centered label                  */
/* ========================================================================= */

#include "overview.h"                              // For overview widget class
#include <QVBoxLayout>                             // For vertical layout
#include <QLabel>                                  // For label widget

// %%% Constructor %%%
/* Initialize overview widget */
Overview::Overview(QWidget *parent)
    : QWidget(parent)
{
    // Create main layout
    QVBoxLayout *layout = new QVBoxLayout(this);
    // Create and configure label
    label = new QLabel("Overview", this);
    label->setAlignment(Qt::AlignCenter);
    // Add label to layout
    layout->addWidget(label);
    // Set widget layout
    setLayout(layout);
}

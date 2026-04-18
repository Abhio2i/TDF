/* ========================================================================= */
/* File: doctrineareadefinition.cpp                                        */
/* Purpose: Implements the Doctrine Area Definition panel widget             */
// Written by   : Arti Rajpoot
/* ========================================================================= */

#include "doctrineareadefinition.h"
#include "doctrine-styles.h"

// %%% Constructor %%%
DoctrineAreaDefinition::DoctrineAreaDefinition(QWidget *parent)
    : QWidget(parent)
{
    setupUI();
    applyStyles();
}

// %%% UI Setup %%%
void DoctrineAreaDefinition::setupUI()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    // %%% Title Bar %%%
    titleLabel = new QLabel("Doctrine Area Definition", this);
    titleLabel->setObjectName("titleLabel");
    titleLabel->setFixedHeight(32);
    mainLayout->addWidget(titleLabel);

    // %%% Divider %%%
    divider = new QFrame(this);
    divider->setObjectName("divider");
    divider->setFrameShape(QFrame::HLine);
    mainLayout->addWidget(divider);

    // %%% Content Area - Empty with just heading as requested %%%
    QWidget *contentWidget = new QWidget(this);
    contentWidget->setStyleSheet("background-color: #0F2636;");

    QVBoxLayout *contentLayout = new QVBoxLayout(contentWidget);
    contentLayout->setContentsMargins(12, 12, 12, 12);

    // Just an empty label to maintain structure, no text content
    contentLabel = new QLabel("", this);
    contentLabel->setAlignment(Qt::AlignTop | Qt::AlignLeft);
    contentLabel->setStyleSheet("color: #A0B0C0; font-size: 12px;");

    contentLayout->addWidget(contentLabel);
    contentLayout->addStretch();

    mainLayout->addWidget(contentWidget);
    mainLayout->addStretch();
}

// %%% Style Application %%%
/* Reuses existing doctrine panel styles */
void DoctrineAreaDefinition::applyStyles()
{
    setStyleSheet(DoctrineStyles::PanelStyle);
}

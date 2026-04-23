/* =============================================================================
 * FILE:         doctrineassumptionsnotes.cpp
 * MODULE:       Doctrine Assumptions & Notes Panel
 * PROJECT:      Indigenous Scenario and Sensor Simulation Toolkit (ISSST)
 * ORGANISATION: Oxygen 2 Innovation (O2I).
 * STANDARD:     RTCA DO-178C / ED-12C, DAL B
 * COVERAGE:     Branch / Decision Coverage required (100% true/false paths)
 *
 * DESCRIPTION:  Implements the DoctrineAssumptionsNotes class which provides a
 *               widget panel for displaying doctrine assumptions and operational
 *               notes. The panel consists of a title label, a visual divider,
 *               and a content label. Styling is applied for consistent UI
 *               presentation across doctrine-related panels.
 *
 * REQUIREMENTS: Implements REQ-DOCTRINE-020 through REQ-DOCTRINE-024
 *
 * AUTHOR:       Arti Rajpoot
 * REVIEWED BY:  [Reviewer Name], [Review Date] — SPR-DOCTRINE-002
 *
 * CHANGE HISTORY:
 *   Rev 1  01 Jan 2026  Initial implementation. Basic panel layout.
 *   Rev 2  20 Apr 2026  DO-178C DAL B compliant comments added throughout.
 *                       Added requirement references and structured file header.
 *
 * COPYRIGHT:    Oxygen 2 Innovation (O2I). All rights reserved.
 *               Restricted circulation — defence simulation use only.
 * =============================================================================
 */

#include "doctrineassumptionsnotes.h"
#include "doctrine-styles.h"

// %%% Constructor %%%
DoctrineAssumptionsNotes::DoctrineAssumptionsNotes(QWidget *parent)
    : QWidget(parent)
{
    setupUI();
    applyStyles();
}

// %%% UI Setup %%%
void DoctrineAssumptionsNotes::setupUI()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    // %%% Title Bar %%%
    titleLabel = new QLabel("Doctrine Assumptions / Notes", this);
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
void DoctrineAssumptionsNotes::applyStyles()
{
    setStyleSheet(DoctrineStyles::PanelStyle);
}

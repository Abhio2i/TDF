/* =============================================================================
 * FILE:         projectinformation.cpp
 * MODULE:       Project Information / Feedback Dialog
 * PROJECT:      Indigenous Scenario and Sensor Simulation Toolkit (ISSST)
 * ORGANISATION: Oxygen 2 Innovation (O2I).
 * STANDARD:     RTCA DO-178C / ED-12C, DAL B
 * COVERAGE:     Branch / Decision Coverage required (100% true/false paths)
 *
 * DESCRIPTION:  Implements the Feedback class which provides a dialog for
 *               displaying project information, version details, and optionally
 *               collecting user feedback. This dialog is typically invoked from
 *               the application's "About" or "Feedback" menu items.
 *
 * REQUIREMENTS: Implements REQ-INFO-010 through REQ-INFO-013
 *
 * AUTHOR:       Arti Rajpoot
 * REVIEWED BY:  [Reviewer Name], [Review Date] — SPR-INFO-001
 *
 *
 * NOTE:         The file is named ProjectInformation.cpp but implements class
 *               Feedback. This is intentional for backward compatibility.
 *
 * COPYRIGHT:    Oxygen 2 Innovation (O2I). All rights reserved.
 *               Restricted circulation — defence simulation use only.
 * =============================================================================
 */
#include "projectinformation.h"
#include "projectinformation-styles.h"
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QFont>
#include <QFrame>


Feedback::Feedback(QWidget *parent)
    : QDialog(parent)
{
    // Apply dark theme to dialog
    setStyleSheet(ProjectInformationStyles::Dialog);
    setWindowTitle("Project Information");
    setFixedSize(500, 280);
    setModal(true);

    // Main layout
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(20);
    mainLayout->setContentsMargins(30, 30, 30, 30);

    // Name section with horizontal layout
    QHBoxLayout *nameLayout = new QHBoxLayout();
    nameLayout->setSpacing(10);

    QLabel *nameTitleLabel = new QLabel("Name:");
    QFont nameTitleFont;
    nameTitleFont.setPointSize(11);
    nameTitleFont.setBold(true);
    nameTitleLabel->setFont(nameTitleFont);
    nameTitleLabel->setStyleSheet(ProjectInformationStyles::TitleLabel);
    nameTitleLabel->setMinimumWidth(80);

    QLabel *projectNameLabel = new QLabel("Indigenous Scenario and Sensor\nSimulation Toolkit");
    QFont projectFont;
    projectFont.setPointSize(11);
    projectNameLabel->setFont(projectFont);
    projectNameLabel->setStyleSheet(ProjectInformationStyles::ProjectNameLabel);
    projectNameLabel->setWordWrap(true);
    nameLayout->addWidget(nameTitleLabel);
    nameLayout->addWidget(projectNameLabel, 1);
    mainLayout->addLayout(nameLayout);

    // Add separator line
    QFrame *line = new QFrame();
    line->setFrameShape(QFrame::HLine);
    line->setStyleSheet(ProjectInformationStyles::Separator);
    mainLayout->addWidget(line);

    // Version section with horizontal layout
    QHBoxLayout *versionLayout = new QHBoxLayout();
    versionLayout->setSpacing(10);

    QLabel *versionTitleLabel = new QLabel("Version:");
    QFont versionTitleFont;
    versionTitleFont.setPointSize(11);
    versionTitleFont.setBold(true);
    versionTitleLabel->setFont(versionTitleFont);
    versionTitleLabel->setStyleSheet(ProjectInformationStyles::TitleLabel);
    versionTitleLabel->setMinimumWidth(80);

    QLabel *versionLabel = new QLabel("5.0.25");
    QFont versionFont;
    versionFont.setPointSize(11);
    versionFont.setBold(true);
    versionLabel->setFont(versionFont);
    versionLabel->setStyleSheet(ProjectInformationStyles::VersionLabel);

    versionLayout->addWidget(versionTitleLabel);
    versionLayout->addWidget(versionLabel, 1);
    mainLayout->addLayout(versionLayout);
    mainLayout->addStretch();
    QPushButton *okButton = new QPushButton("OK");
    okButton->setFixedSize(70, 28);
    okButton->setCursor(Qt::PointingHandCursor);
    okButton->setStyleSheet(ProjectInformationStyles::OkButton);
    connect(okButton, &QPushButton::clicked, this, &QDialog::accept);
    mainLayout->addWidget(okButton, 0, Qt::AlignCenter);
}


/* =============================================================================
 * FILE:         ProjectInformation.h
 * MODULE:       Project Information / Feedback Dialog
 * PROJECT:      Indigenous Scenario and Sensor Simulation Toolkit (ISSST)
 * ORGANISATION: Oxygen 2 Innovation (O2I).
 * STANDARD:     RTCA DO-178C / ED-12C, DAL B
 * COVERAGE:     Branch / Decision Coverage required (100% true/false paths)
 *
 * DESCRIPTION:  Declares the Feedback class which provides a dialog for
 *               displaying project information, version details, and optionally
 *               collecting user feedback. This dialog is typically invoked from
 *               the application's "About" or "Feedback" menu items.
 *
 * REQUIREMENTS: REQ-INFO-010  Display project information dialog
 *               REQ-INFO-011  Show application version and copyright
 *               REQ-INFO-013  Modal dialog with OK/Close button
 *
 * AUTHOR:       Arti Rajpoot
 * REVIEWED BY:  [Reviewer Name], [Review Date] — SPR-INFO-001
 *
 *
 * NOTE:         The file is named ProjectInformation.h but declares class Feedback.
 *               This is intentional for backward compatibility.
 *
 * COPYRIGHT:    Oxygen 2 Innovation (O2I). All rights reserved.
 *               Restricted circulation — defence simulation use only.
 * =============================================================================
 */
#ifndef PROJECTINFORMATION_H
#define PROJECTINFORMATION_H

#include <QDialog>

class Feedback : public QDialog
{
    Q_OBJECT
public:
    explicit Feedback(QWidget *parent = nullptr);
};

#endif // PROJECTINFORMATION_H

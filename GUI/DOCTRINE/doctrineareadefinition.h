/* =============================================================================
 * FILE:         doctrineareadefinition.h
 * MODULE:       Doctrine Area Definition Panel
 * PROJECT:      Indigenous Scenario and Sensor Simulation Toolkit (ISSST)
 * ORGANISATION: Oxygen 2 Innovation (O2I).
 * STANDARD:     RTCA DO-178C / ED-12C, DAL B
 * COVERAGE:     Branch / Decision Coverage required (100% true/false paths)
 *
 * DESCRIPTION:  Declares the DoctrineAreaDefinition class which provides a
 *               widget panel for displaying doctrine area definition information.
 *               The panel consists of a title label, a visual divider, and a
 *               content label. Styling is applied for consistent UI presentation.
 *
 * REQUIREMENTS: REQ-DOCTRINE-010  Doctrine area definition panel display
 *               REQ-DOCTRINE-011  Title label with doctrine area name
 *               REQ-DOCTRINE-012  Content area for doctrinal text/definition
 *               REQ-DOCTRINE-013  Visual separator (divider) between title/content
 *               REQ-DOCTRINE-014  Consistent styling across all doctrine panels
 *
 * AUTHOR:       Arti Rajpoot
 * REVIEWED BY:  [Reviewer Name], [Review Date] — SPR-DOCTRINE-001
 *
 *
 * COPYRIGHT:    Oxygen 2 Innovation (O2I). All rights reserved.
 *               Restricted circulation — defence simulation use only.
 * =============================================================================
 */

#ifndef DOCTRINEAREADEFINITION_H
#define DOCTRINEAREADEFINITION_H

#include <QWidget>
#include <QLabel>
#include <QVBoxLayout>
#include <QFrame>

// %%% Class Definition %%%
/* Panel widget for displaying doctrine area definition */
class DoctrineAreaDefinition : public QWidget
{
    Q_OBJECT

public:
    explicit DoctrineAreaDefinition(QWidget *parent = nullptr);
    ~DoctrineAreaDefinition() = default;

private:
    // %%% UI Setup Methods %%%
    void setupUI();
    void applyStyles();

    // %%% UI Components %%%
    QLabel *titleLabel = nullptr;
    QFrame *divider = nullptr;
    QLabel *contentLabel = nullptr;
};

#endif // DOCTRINEAREADEFINITION_H

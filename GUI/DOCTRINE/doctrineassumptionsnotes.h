/* =============================================================================
 * FILE:         doctrineassumptionsnotes.h
 * MODULE:       Doctrine Assumptions & Notes Panel
 * PROJECT:      Indigenous Scenario and Sensor Simulation Toolkit (ISSST)
 * ORGANISATION: Oxygen 2 Innovation (O2I).
 * STANDARD:     RTCA DO-178C / ED-12C, DAL B
 * COVERAGE:     Branch / Decision Coverage required (100% true/false paths)
 *
 * DESCRIPTION:  Declares the DoctrineAssumptionsNotes class which provides a
 *               widget panel for displaying doctrine assumptions and operational
 *               notes. The panel consists of a title label, a visual divider,
 *               and a content label. Styling is applied for consistent UI
 *               presentation across doctrine-related panels.
 *
 * REQUIREMENTS: REQ-DOCTRINE-020  Doctrine assumptions/notes panel display
 *               REQ-DOCTRINE-021  Title label for assumptions section
 *               REQ-DOCTRINE-022  Content area for assumption text/notes
 *               REQ-DOCTRINE-023  Visual separator between title and content
 *               REQ-DOCTRINE-024  Consistent styling with DoctrineAreaDefinition
 *
 * AUTHOR:       Arti Rajpoot
 * REVIEWED BY:  [Reviewer Name], [Review Date] — SPR-DOCTRINE-002
 *
 *
 * COPYRIGHT:    Oxygen 2 Innovation (O2I). All rights reserved.
 *               Restricted circulation — defence simulation use only.
 * =============================================================================
 */

#ifndef DOCTRINEASSUMPTIONSNOTES_H
#define DOCTRINEASSUMPTIONSNOTES_H

#include <QWidget>
#include <QLabel>
#include <QVBoxLayout>
#include <QFrame>

// %%% Class Definition %%%
/* Panel widget for displaying doctrine assumptions and notes */
class DoctrineAssumptionsNotes : public QWidget
{
    Q_OBJECT

public:
    explicit DoctrineAssumptionsNotes(QWidget *parent = nullptr);
    ~DoctrineAssumptionsNotes() = default;

private:
    // %%% UI Setup Methods %%%
    void setupUI();
    void applyStyles();

    // %%% UI Components %%%
    QLabel *titleLabel = nullptr;
    QFrame *divider = nullptr;
    QLabel *contentLabel = nullptr;
};

#endif // DOCTRINEASSUMPTIONSNOTES_H

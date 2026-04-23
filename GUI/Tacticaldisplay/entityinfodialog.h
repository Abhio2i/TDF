/* =============================================================================
 * FILE:         entityinfodialog.h
 * MODULE:       Entity Information Dialog
 * PROJECT:      Indigenous Scenario and Sensor Simulation Toolkit (ISSST)
 * ORGANISATION: Oxygen 2 Innovation (O2I).
 * STANDARD:     RTCA DO-178C / ED-12C, DAL B
 * COVERAGE:     Branch / Decision Coverage required (100% true/false paths)
 *
 * DESCRIPTION:  Declares the EntityInfoDialog class which provides a modal
 *               dialog for displaying and managing entity information.
 *               Shows entity attributes, position, speed/altitude table,
 *               equipment (sensors, radios, IFF, weapons, formation),
 *               and options (track, centre, aggregated script, follow
 *               trajectory, show connection, show detection). Integrates
 *               with CanvasWidget, Hierarchy, Platform, Formation, and
 *               MeshEntry to display real‑time entity data. Supports
 *               editing speed/altitude values via table cells and emits
 *               signals when updates occur.
 *
 * REQUIREMENTS: REQ-ENTITYINFO-010  Entity information dialog
 *               REQ-ENTITYINFO-011  Display entity ID, name, and mesh info
 *               REQ-ENTITYINFO-012  Attribute section (type, category, team)
 *               REQ-ENTITYINFO-013  Position section (latitude, longitude, altitude)
 *               REQ-ENTITYINFO-014  Speed/Altitude table with editable cells
 *               REQ-ENTITYINFO-015  Equipment section: sensors, radios, IFF,
 *                                   weapons, formation buttons
 *               REQ-ENTITYINFO-016  Options section: track, centre, aggregated
 *                                   script, follow trajectory, show connection,
 *                                   show detection
 *               REQ-ENTITYINFO-017  Clear displayed information
 *               REQ-ENTITYINFO-018  Signal speedAltitudeUpdated when speed/alt
 *                                   table is edited
 *
 * AUTHOR:       Arti Rajpoot
 * REVIEWED BY:  [Reviewer Name], [Review Date] — SPR-ENTITYINFO-001
 *
 *
 * COPYRIGHT:    Oxygen 2 Innovation (O2I). All rights reserved.
 *               Restricted circulation — defence simulation use only.
 * =============================================================================
 */

#ifndef ENTITYINFODIALOG_H
#define ENTITYINFODIALOG_H

#include "GUI/Tacticaldisplay/canvaswidget.h"
#include "core/Hierarchy/hierarchy.h"
#include "core/Hierarchy/EntityProfiles/formation.h"
#include "core/Hierarchy/Struct/formationposition.h"
#include "core/Hierarchy/EntityProfiles/platform.h"
#include <QDialog>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QLabel>
#include <QScrollArea>
#include <QCheckBox>
#include <QPushButton>
#include <QVariantMap>
#include <QTableWidget>

class CanvasWidget;

// %%% Class Definition %%%
/* Dialog for displaying and controlling entity properties and status */
class EntityInfoDialog : public QDialog
{
    Q_OBJECT

public:
    // %%% Constructor %%%
    /* Initialize entity information dialog */
    explicit EntityInfoDialog(QWidget *parent = nullptr);
    void setEntityInfo(const QString& entityId, const QString& entityName, MeshEntry* info);
    void updateEntityInfo();
    /* Clear all displayed information */
    void clearInfo();

signals:
    // %%% Update Signal %%%
    /* Signal to request external updates */
    void update();
    void speedAltitudeUpdated(const QString& entityId, float speed, float altitude);

private slots:
    // %%% Action Slots %%%
    /* Handle close button click */
    void onCloseClicked();
    /* Handle sensors button click */
    void onSensorsClicked();
    /* Handle radios button click */
    void onRadiosClicked();
    /* Handle IFF button click */
    void onIFFClicked();
    /* Handle weapons button click */
    void onWeaponsClicked();
    /* Handle formation button click */
    void onFormationClicked();
     void onSpeedAltCellChanged(int row, int column);
private:
    // %%% UI Setup Methods %%%
    /* Set up user interface */
    void setupUI();
    /* Create attribute section */
    void createAttributeSection();
    /* Create position section */
    void createPositionSection();
    /* Create speed/altitude table */
    void createSpeedAltTableSection();
    /* Create track section */
    // void createTrackSection();
    /* Create equipment section */
    void createEquipmentSection();
    /* Create options section */
    void createOptionsSection();
   QString currentEntityName;
    // %%% Data Update Methods %%%
    /* Update sensors table data */
    void updateSensorsTable(QTableWidget* sensorsTable, Entity* platform);
    /* Update radios table data */
    void updateRadiosTable(QTableWidget* radiosTable, Entity* platform);
    void updateFormationTable(QTableWidget* formationTable, Entity* entity);
    // %%% Data Members %%%
    MeshEntry* entryInfo = nullptr;
    QString currentEntityId;

    // %%% UI Component Members %%%
    QVBoxLayout *mainLayout = nullptr;
    QLabel *titleLabel = nullptr;
    QScrollArea *scrollArea = nullptr;
    QWidget *scrollWidget = nullptr;
    QVBoxLayout *scrollLayout = nullptr;
    QPushButton *closeButton = nullptr;

    // Table widgets
    QTableWidget *attributeTable = nullptr;
    QTableWidget *speedAltTable = nullptr;

    // Other widgets
    QLabel *positionLabel = nullptr;
    QLabel *carrierLabel = nullptr;

    // Track section
    QCheckBox *trackCheckBox = nullptr;
    QCheckBox *centreCheckBox = nullptr;
    QCheckBox *aggregatedScriptCheckBox = nullptr;

    // Equipment section
    QPushButton *weaponsButton = nullptr;
    QPushButton *sensorsButton = nullptr;
    QPushButton *formationButton = nullptr;
    QPushButton *radiosButton = nullptr;
    QPushButton *iffButton = nullptr;

    // Options section
    QCheckBox *followTrajectoryCheckBox = nullptr;
    QCheckBox *showConnectionCheckBox = nullptr;
    QCheckBox *showDetectionCheckBox = nullptr;
    Formation* findFormationForEntity(Entity* entity);
    void displayFormationInfo(Formation* formation, Entity* currentEntity, QVBoxLayout* layout);

};

#endif // ENTITYINFODIALOG_H

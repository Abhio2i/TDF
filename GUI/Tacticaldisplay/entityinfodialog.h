/* ========================================================================= */
/* File: entityinfodialog.h                                                 */
/* Purpose: Dialog for displaying and managing entity information           */
//               Written by Arti Rajpoot
/* ========================================================================= */

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

    // %%% Entity Management Methods %%%
    /* Set entity information for display */
    // void setEntityInfo(const QString& entityId, MeshEntry *info);
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

/* =============================================================================
 * FILE:         ESMDisplay.h
 * MODULE:       ESM (Electronic Support Measures) Display
 * PROJECT:      Indigenous Scenario and Sensor Simulation Toolkit (ISSST)
 * ORGANISATION: Oxygen 2 Innovation (O2I).
 * STANDARD:     RTCA DO-178C / ED-12C, DAL B
 * COVERAGE:     Branch / Decision Coverage required (100% true/false paths)
 *
 * DESCRIPTION:  Declares the ESMDisplay class: the ESM panel. It hosts two
 *               selectable display modes (PPI scope and Signal Spectrum) and
 *               one observation table shared by both. The circular PPI is
 *               drawn by ESMPpiWidget, the spectrum graph by
 *               ESMSpectrumWidget, and both tables/plots read the same
 *               ESMObservationModel. ESMDisplay keeps the public API used by
 *               the rest of the application (selectEntity, RemoveEntity,
 *               updateRadar, ...) and owns the PPI <-> table selection sync.
 *
 * REQUIREMENTS: REQ-ESM-010  ESM visualisation widget
 *               REQ-ESM-011  Display targets with angle and radius
 *               REQ-ESM-012  Configurable range and ring count
 *               REQ-ESM-013  Draw radar rings, ticks, labels, centre mark
 *               REQ-ESM-014  Mouse hover detection over targets
 *               REQ-ESM-015  Integration with Hierarchy and Sensor entities
 *               REQ-ESM-016  Update display on entity selection and removal
 *               REQ-ESM-017  (Rev 3) PPI keeps a 1:1 aspect (circular scope)
 *                            inside a resizable panel; the former 16:9
 *                            height-for-width rule applied to the bare scope
 *
 * AUTHOR:       Arti Rajpoot
 * REVIEWED BY:  [Reviewer Name], [Review Date] — SPR-ESM-001
 *
 *
 * COPYRIGHT:    Oxygen 2 Innovation (O2I). All rights reserved.
 *               Restricted circulation — defence simulation use only.
 * =============================================================================
 */
#ifndef ESMDisplay_H
#define ESMDisplay_H

#include "core/Hierarchy/EntityProfiles/SensorProfiles/esm.h"
#include "core/Hierarchy/EntityProfiles/sensor.h"  // For sensor profile
#include "core/Hierarchy/hierarchy.h"             // For hierarchy data structure
#include <QWidget>                                // For widget base class
#include <QVector>                                // For vector container
#include <QComboBox>
#include <core/Debug/profiler.h>

class QLabel;
class QPushButton;
class QSplitter;
class QStackedWidget;
class QTableView;
class ESMObservationModel;
class ESMPpiWidget;
class ESMSpectrumWidget;

// %%% Class Definition %%%
/* ESM panel: PPI / Signal Spectrum modes + shared observation table */
class ESMDisplay : public QWidget
{
    Q_OBJECT

public:
    // Initialize ESM panel
    explicit ESMDisplay(QWidget *parent = nullptr);
    // Set hierarchy instance
    void setHierarchy(Hierarchy* h) { hierarchy = h; }
    // Get size hint
    QSize sizeHint() const override;
    // Configured ESM range in km (mirrors sensor->range)
    int range = 100;
    // Set radar range
    void setRange(float value);
    // Select entity
    void selectEntity(Entity* entity);
    // Remove entity by ID
    void RemoveEntity(QString ID);
    // Update display from the current sensor detections
    void updateRadar();
    // Sensor instance
    ESM* sensor = nullptr;
    QVector<ESM*> sensorlist;
    // Entity platform
    Platform* entity = nullptr;

    // Hook for the future ESM processing modules: feed measured spectrum here
    // via ESMSpectrumWidget::setSpectrum().
    ESMSpectrumWidget* spectrumWidget() const { return spectrum; }

private slots:
    void onSensorSelected(int index);
    void onPpiObservationSelected(const QString& key);
    void onTableSelectionChanged();
    void setMode(int mode);

private:
    // %%% Helpers %%%
    void buildUi();
    void collectSensors();
    void updateDropdown();
    void refreshObservations();
    void applySelection(bool scrollToSelection);
    void updateSelectedInfo();
    void updateStatusLabel();

    // %%% State %%%
    // Entity ID
    QString id = "";
    // Hierarchy instance
    Hierarchy* hierarchy = nullptr;
    // Key (platform ID) of the selected observation, shared by PPI and table
    QString selectedKey;
    bool syncing = false;

    // %%% Child widgets (owned by Qt parent/child) %%%
    ESMObservationModel* model = nullptr;
    ESMPpiWidget* ppi = nullptr;
    ESMSpectrumWidget* spectrum = nullptr;
    QTableView* table = nullptr;
    QStackedWidget* stack = nullptr;
    QSplitter* splitter = nullptr;
    QPushButton* ppiButton = nullptr;
    QPushButton* spectrumButton = nullptr;
    QLabel* statusLabel = nullptr;
    QLabel* observationsLabel = nullptr;
    QComboBox* sensorDropdown = nullptr;
};

#endif // ESMDisplay_H

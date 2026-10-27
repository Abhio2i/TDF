/* =============================================================================
 * FILE:         CSMDisplay.h
 * MODULE:       CSM (Communications Support Measures) Display
 * PROJECT:      Indigenous Scenario and Sensor Simulation Toolkit (ISSST)
 * ORGANISATION: Oxygen 2 Innovation (O2I).
 * STANDARD:     RTCA DO-178C / ED-12C, DAL B
 * COVERAGE:     Branch / Decision Coverage required (100% true/false paths)
 *
 * DESCRIPTION:  Declares the CSMDisplay class which provides a widget for
 *               visualising Communications Support Measures (CSM) data.
 *               The widget has two sections:
 *                 1. FREQUENCY SPECTRUM - spectrum frame (axes, grid, monitor-mode
 *                    indicator) with a green PSD trace [dBm/Hz] taken from
 *                    CSM::lastSpectrum(). The trace is a MODELLED spectrum
 *                    (labelled "MODELLED / SIMULATED - NOT MEASURED"). If the
 *                    model cannot be evaluated, the reason is shown instead.
 *                    Double-click toggles full monitor span / zoom on the
 *                    strongest signal.
 *                 2. OBSERVATIONS - table (# | FREQUENCY | BANDWIDTH |
 *                    MODULATION | LEVEL | EMITTER) with one row per detected
 *                    RADIO, taken from CSM::lastScanResult().
 *               Integrates with Hierarchy and Sensor/Platform entities.
 *
 * DATA POLICY:  Only values available from the backend are shown; anything
 *               else is "--". The display performs no detection or
 *               propagation maths of its own.
 *                 FREQUENCY / BANDWIDTH / MODULATION: CONFIGURED settings of
 *                 the detected radio (Provenance::Configured), NOT values
 *                 measured or classified by the CSM.
 *                 LEVEL: CALCULATED link-budget received level in dBm
 *                 (Provenance::Calculated), a model result from the existing
 *                 propagation model, not a measurement and not a PSD. "--"
 *                 when any input is unavailable.
 *                 EMITTER: platform/radio names of the detected radio.
 *                 SPECTRUM: modelled PSD (csmspectrum.h), never a measurement.
 *                 The display only draws the cached result; it computes nothing.
 *
 * REQUIREMENTS: REQ-CSM-010  CSM visualisation widget
 *               REQ-CSM-015  Integration with Hierarchy and Sensor entities
 *               REQ-CSM-016  Update display on entity selection and removal
 *               REQ-CSM-017  Maintain aspect ratio (16:9)
 *               NOTE: REQ-CSM-011/012/013/014 describe the former polar
 *               display (angle/radius targets, rings, ticks, hover). That
 *               behaviour is replaced by the spectrum/table layout; the
 *               requirement baseline needs updating by the document owner.
 *
 * AUTHOR:       Arti Rajpoot
 * REVIEWED BY:  [Reviewer Name], [Review Date] — SPR-CSM-001
 *
 * COPYRIGHT:    Oxygen 2 Innovation (O2I). All rights reserved.
 *               Restricted circulation — defence simulation use only.
 * =============================================================================
 */

#ifndef CSMDisplay_H
#define CSMDisplay_H

#include "core/Hierarchy/EntityProfiles/sensor.h"  // For sensor profile
#include "core/Hierarchy/hierarchy.h"             // For hierarchy data structure
#include <QWidget>                                // For widget base class
#include <QVector>                                // For vector container
#include <QStringList>                            // For table rows
#include <core/Debug/profiler.h>
#include <QComboBox>
#include <memory>
#include "core/Hierarchy/EntityProfiles/SensorProfiles/CSM/csmspectrum.h"
#include "core/Hierarchy/EntityProfiles/SensorProfiles/CSM/csmmonitor.h"

// %%% Data Structures %%%
/* Structure for electronic warfare target */
struct CSMTarget {
    double angle = 0.0;                           // Target angle
    double radius = 0.0;                          // Target radius
};

// %%% Class Definition %%%
/* Widget for CSM frequency-spectrum and observations display */
class CSMDisplay : public QWidget
{
    Q_OBJECT

public:
    // Initialize CSM display
    explicit CSMDisplay(QWidget *parent = nullptr);
    // Set hierarchy instance
    void setHierarchy(Hierarchy* h) { hierarchy = h; }
    // Get size hint
    QSize sizeHint() const override;
    // Get minimum size
    QSize minimumSize() const;
    int range = 100;
    // Set sensor range (kept for compatibility; not used for drawing)
    void setRange(float value) { range = value; }
    // Get height for width
    int heightForWidth(int width) const override;
    // Select entity
    void selectEntity(Entity* entity);
    // Remove entity by ID
    void RemoveEntity(QString ID);
    // Refresh display from the selected CSM sensor (called every sim update)
    void updateRadar();
    // Sensor instance
    Sensor* sensor = nullptr;
    QVector<Sensor*> sensorlist;
    // Entity platform
    Platform* entity = nullptr;

protected:
    // Handle paint events
    void paintEvent(QPaintEvent *event) override;
    // Kept for compatibility; no hover behaviour in the spectrum layout
    void mouseMoveEvent(QMouseEvent *event) override;
    // Toggle full-span / zoom spectrum view
    void mouseDoubleClickEvent(QMouseEvent *event) override;
    // Handle mouse leave
    void leaveEvent(QEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    // Scroll the observations table when it has more rows than fit
    void wheelEvent(QWheelEvent *event) override;
private slots:
    void onSensorSelected(int index);
private:
    // %%% Display Properties %%%
    // Aspect ratio for display
    const double ASPECT_RATIO = 16.0/9.0;
    // Outer margin
    int padding = 6;
    // Radar green color
    QColor radarGreen = QColor(0, 255, 0);
    // Entity ID
    QString id = "";
    // Hierarchy instance
    Hierarchy* hierarchy = nullptr;

    // %%% Cached state (rebuilt in updateRadar, read in paintEvent) %%%
    // Observation rows: { #, FREQUENCY, BANDWIDTH, MODULATION, LEVEL, EMITTER }
    QVector<QStringList> rows;
    // True if any row shows configured radio values
    bool showsConfiguredValues = false;
    // True if any row shows a calculated (link-budget) level
    bool showsCalculatedLevel = false;
    // Selected sensor exists and is Active
    bool sensorActive = false;
    // At least one update tick has been seen while the sensor was Active.
    // The backend has no "scan completed" flag; this is the closest proxy.
    bool scanObserved = false;
    // First visible table row and number of rows that fit (set by paint)
    int rowScroll = 0;
    int visibleRows = 0;
    // Modelled spectrum of the selected sensor (shared, immutable) and view mode
    std::shared_ptr<const csm::spectrum::Result> spectrum_;
    csm::monitor::Window monitor_;   // monitoring window of the last scan (for labels)
    bool zoomView_ = false;
    // Mouse-wheel zoom: display-only; does not change monitor configuration.
    bool viewZoomed_ = false;
    double viewLoHz_ = 0.0;
    double viewHiHz_ = 0.0;

    QRect spectrumPlot_;

    static constexpr double kMinViewHz = 10.0e3;
    static constexpr double kWheelStep = 0.8;

    bool applyWheelZoom(double notches, int xPixel);
    bool effectiveView(const csm::spectrum::Result &res,double &lo,double &hi) const;
    QComboBox* sensorDropdown = nullptr;
    void updateDropdown();
    void layoutChildren();

    // %%% Data Methods %%%
    // Rebuild cached rows/state. Returns true if anything visible changed.
    bool refreshObservations(bool scanTick);
    // Message shown in the empty observations table
    QString observationsMessage() const;
    // Display font scaled to the widget width
    QFont displayFont(int delta = 0, bool bold = false) const;

    // %%% Drawing Methods %%%
    // Frequency spectrum section
    void drawSpectrum(QPainter &p, const QRect &r);
    // Spectrum trace, labels and provenance inside the plot rectangle
    void drawSpectrumTrace(QPainter &p, const QRect &plot);
    // Header mode text derived from the sensor's real configuration
    QString modeLabel() const;
    // Observations table section
    void drawObservations(QPainter &p, const QRect &r);
    // Split widget into spectrum and observations rectangles
    void sectionRects(QRect &spectrum, QRect &observations) const;
};

#endif // CSMDisplay_H

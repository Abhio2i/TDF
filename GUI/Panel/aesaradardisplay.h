// =============================================================================
// FILE:         aesaradardisplay.h
// MODULE:       AESA Radar Display — Qt Widget
// PROJECT:      Indigenous Scenario and Sensor Simulation Toolkit (ISSST)
// ORGANISATION: Oxygen to Innovation Pvt. Ltd.
// STANDARD:     RTCA DO-178C / ED-12C, DAL B
// COVERAGE:     Branch / Decision Coverage required (100% true/false paths)
//
// DESCRIPTION:  Declares AESARadarDisplay, a QWidget-derived radar scope
//               that mirrors RadarDisplay structure exactly and adds the
//               following AESA-specific display capabilities:
//
//                 - IFF-coloured target symbols: cyan = FRIENDLY, red = HOSTILE,
//                   yellow = UNKNOWN. Derived from iffResult signal data.
//                 - DRFM ghost warning banner (amber) displayed for ~60 frames
//                   after drfmGhostDetected is received from AESARadar.
//                 - T/R module duty cycle bar (green / amber / red) populated
//                   each frame from schedulerDutyCycle signal.
//                 - Track quality indicator shown in target data label.
//                 - Two display modes: AIR (sector sweep, bottom-centre origin)
//                   and SURFACE (full 360° PPI, centre origin).
//                 - Four mode buttons: SURV, TWS, LOCK, AIR/SURF toggle.
//                 - Mouse left-click target selection for fire-control lock-on.
//
//               All physics and tracking remain in the model layer
//               (RadarModel_AESA). This class is pure display and input
//               forwarding — no radar computations reside here.
//
// REQUIREMENTS: REQ-AESA-003  Mode control (SURV / TWS / LOCK button actions)
//               REQ-AESA-004  Output display (target rendering from sensor->targets)
//               REQ-AESA-020  Duty cycle display (schedulerDutyCycle signal)
//               REQ-AESA-050  IFF colouring (iffResult signal)
//               REQ-AESA-060  DRFM warning banner (drfmGhostDetected signal)
//
// AUTHOR:       Oxygen to Innovation Pvt. Ltd.
// REVIEWED BY:  [Reviewer Name], [Review Date] — SPR-AESA-DISPLAY-001
//
// CHANGE HISTORY:
//   Rev 1  01 Jan 2026  Initial implementation. Basic air / surface modes.
//   Rev 2  15 Feb 2026  IFF colouring, DRFM banner, duty cycle bar added.
//                       Lock reticle and mouse-click lock-on added.
//                       Track quality bar in label added.
//   Rev 3  01 Apr 2026  TWS coasted track sector filtering fixed — validated
//                       tracks shown even outside scan sector. FNV-1a hash
//                       used for target ID resolution matching aesaradar.cpp.
//   Rev 4  20 Apr 2026  DO-178C DAL B compliant comments added throughout.
//
// COPYRIGHT:    Oxygen to Innovation Pvt. Ltd. All rights reserved.
//               Restricted circulation — defence simulation use only.
// =============================================================================

#ifndef AESARADARDISPLAY_H
#define AESARADARDISPLAY_H

#include "core/Hierarchy/EntityProfiles/SensorProfiles/aesaradar.h"
#include "core/Hierarchy/EntityProfiles/sensor.h"

#include <QWidget>
#include <QPainter>
#include <QJsonObject>
#include <QVector>
#include <QPushButton>
#include <QLabel>
#include <QMouseEvent>
#include <QComboBox>
// =============================================================================
// CLASS: AESARadarDisplay
//
// DESCRIPTION:  Qt widget implementing the AESA radar operator scope.
//               Renders target symbols, range rings, scan sector lines, HUD
//               data, IFF colour coding, DRFM warnings, duty cycle bar, and
//               the fire-control lock reticle. Mode buttons allow the operator
//               to switch between SURVEILLANCE, TWS, and LOCK_ON modes.
//               Mouse left-click on a target symbol initiates fire-control
//               lock-on via AESARadar::lockOn().
//
//               Two display modes:
//                 AIR     — half-plane sector sweep, origin at bottom-centre.
//                           Suitable for air search and track display.
//                 SURFACE — full 360° PPI (Plan Position Indicator), origin
//                           at centre. Suitable for surface / 360° scenarios.
//
//               The widget maintains screenTargets, a list rebuilt on every
//               paintEvent mapping each Sensor::Target to a screen position
//               and metadata (IFF code, DRFM flag, track quality). This list
//               is also used by mousePressEvent for AESA_HIT-radius hit-testing.
//
// THREAD SAFETY: Not thread-safe. All methods must be called from the Qt
//                main thread (GUI thread). updateRadar() is expected to be
//                called each simulation tick from the engine's main thread.
//
// REQUIREMENTS: REQ-AESA-003, REQ-AESA-004, REQ-AESA-020,
//               REQ-AESA-050, REQ-AESA-060.
//
// TRACEABILITY:
//   Interfaces: AESARadar (aesaradar.h) — signal connections in selectEntity()
//   Engine:     Called from scene manager via selectEntity() / updateRadar()
// =============================================================================
class AESARadarDisplay : public QWidget
{
    Q_OBJECT

public:

    // =========================================================================
    // ENUMERATION: DisplayMode
    //
    // DESCRIPTION: Controls the geometry of the radar scope display.
    //   AIR     — Half-plane sweep, origin at bottom-centre of display area.
    //             Azimuth sector bounded by scanMinAz / scanMaxAz.
    //   SURFACE — Full 360° PPI, origin at display area centre.
    //             North-up N/S/E/W cardinal labels on outer ring.
    // REQUIREMENT: REQ-AESA-004
    // =========================================================================
    enum class DisplayMode { AIR, SURFACE };

    // =========================================================================
    // CONSTRUCTOR: AESARadarDisplay
    //
    // DESCRIPTION: Constructs the display widget. Creates the four mode buttons
    //              (SURV, TWS, LOCK, AIR/SURF), applies initial button styles,
    //              connects button clicked signals to onModeButtonClicked and
    //              toggleDisplayMode slots. Sets background black and enables
    //              mouse tracking.
    //
    // REQUIREMENT: REQ-AESA-004
    //
    // PARAMETERS:
    //   parent  [in]  Qt parent widget. May be null (top-level window).
    //
    // SIDE EFFECTS: Heap-allocates four QPushButton objects (Qt-parented,
    //               lifetime tied to this widget).
    // =========================================================================
    explicit AESARadarDisplay(QWidget* parent = nullptr);

    // =========================================================================
    // FUNCTION:    setHierarchy
    //
    // DESCRIPTION: Stores a pointer to the engine Hierarchy for future scene-
    //              graph access. Must be called before selectEntity() if the
    //              widget needs to query the hierarchy directly. REQ-AESA-004.
    //
    // PARAMETERS:
    //   h  [in]  Hierarchy pointer. May be null.
    // =========================================================================
    void setHierarchy(Hierarchy* h) { hierarchy = h; }

    // =========================================================================
    // FUNCTION:    selectEntity
    //
    // DESCRIPTION: Binds the display to the AESA radar sensor on the given
    //              platform entity. Searches entity->sensors for a sensor with
    //              subType == SubType::AESA. If found:
    //                - Reads scanMinAz / scanMaxAz from RadarConfig.
    //                - Connects AESARadar::iffResult, drfmGhostDetected, and
    //                  schedulerDutyCycle signals (Qt::UniqueConnection).
    //                - Sets display mode: SURFACE if FoV >= 360°, AIR otherwise.
    //                - Updates window title to platform entity name.
    //              Clears all cached state (targets, iffMap_, lock) on entry.
    //
    // REQUIREMENT: REQ-AESA-003, REQ-AESA-004
    //
    // PARAMETERS:
    //   entity  [in]  Engine entity to bind to. If dynamic_cast to Platform*
    //                 fails, updates display state and returns without binding.
    //
    // SIDE EFFECTS: Sets this->sensor, this->entity, lockedTargetID, iffMap_,
    //               scanMinAz, scanMaxAz. Connects Qt signals. Triggers repaint.
    // =========================================================================
    void selectEntity(Entity* entity);

    // =========================================================================
    // FUNCTION:    RemoveEntity
    //
    // DESCRIPTION: Called by the engine when a platform is removed from the
    //              scene. If the removed entity's ID matches this->id, clears
    //              all display state to defaults and repaints with empty display.
    //
    // REQUIREMENT: REQ-AESA-004
    //
    // PARAMETERS:
    //   ID  [in]  String identifier of the removed entity.
    //
    // SIDE EFFECTS: If matched: sets entity = nullptr, sensor = nullptr, clears
    //               targets, screenTargets, lockedTargetID, iffMap_. Repaints.
    // =========================================================================
    void RemoveEntity(QString ID);

    // =========================================================================
    // FUNCTION:    updateRadar
    //
    // DESCRIPTION: Called each simulation tick to refresh the display. Re-reads
    //              scanMinAz / scanMaxAz from RadarConfig (may change at runtime
    //              if config is updated). Decrements drfmWarnFrames_ countdown.
    //              Updates mode button styles. Triggers repaint via update().
    //
    // REQUIREMENT: REQ-AESA-004
    //
    // SIDE EFFECTS: Reads RadarConfig. Decrements drfmWarnFrames_ if > 0.
    //               Calls updateModeButtonStyles(). Calls update().
    // =========================================================================
    void updateRadar();

    // =========================================================================
    // FUNCTION:    setDisplayMode
    //
    // DESCRIPTION: Sets the display geometry mode and updates the AIR/SURF
    //              button label. Triggers repaint. REQ-AESA-004.
    //
    // PARAMETERS:
    //   m  [in]  New display mode (AIR or SURFACE).
    //
    // SIDE EFFECTS: Sets displayMode. Updates btnDispMode->text(). Calls update().
    // =========================================================================
    void setDisplayMode(DisplayMode m);

    // =========================================================================
    // SIZE POLICY OVERRIDES
    // =========================================================================

    // Returns preferred widget size. Width = 520, height = 520 × AESA_ASPECT.
    // REQ-AESA-004.
    QSize sizeHint()            const override;

    // Returns minimum acceptable widget size. Width = 200.
    QSize minimumSize()         const;

    // Returns height for a given width maintaining AESA_ASPECT (16/9).
    // Required by Qt's heightForWidth layout policy. REQ-AESA-004.
    int   heightForWidth(int w) const override;

    // =========================================================================
    // PUBLIC STATE
    // Accessed directly by the engine / scene manager each tick.
    // =========================================================================

    // Active sensor bound to this display. Null if no entity is selected or
    // the entity has no AESA sensor. REQ-AESA-004.
    // Active sensor bound to this display. Null if no entity is selected or
    // the entity has no AESA sensor. REQ-AESA-004.
    Sensor*   sensor = nullptr;

    // =========================================================================
    // PUBLIC STATE: sensorlist
    //
    // DESCRIPTION: List of all AESA sensors found on the currently bound
    //              platform entity. Populated by selectEntity() each time a
    //              new entity is bound. Contains one entry per AESA-subtype
    //              sensor on the platform. If the platform has only one AESA
    //              sensor the dropdown is hidden; if two or more are present
    //              the dropdown is shown to allow operator selection.
    //              Cleared by RemoveEntity() when the platform leaves the scene.
    //
    // REQUIREMENT: REQ-AESA-004
    // =========================================================================
    QVector<Sensor*> sensorlist;

    // Bound platform entity. Null until selectEntity() is called. REQ-AESA-004.
    Platform* entity = nullptr;

protected:

    // =========================================================================
    // Qt EVENT OVERRIDES
    // =========================================================================

    // =========================================================================
    // FUNCTION:    paintEvent  (override)
    //
    // DESCRIPTION: Main render dispatch. In AIR mode calls drawAirRings,
    //              drawAirSector, drawCenterMark, drawAirTargets. In SURFACE
    //              mode calls drawSurfaceRings, drawCenterMark,
    //              drawSurfaceTargets. Then overlays drawLockReticle,
    //              drawDutyCycleBar, drawDRFMWarning, drawHUD. REQ-AESA-004.
    // =========================================================================
    void paintEvent     (QPaintEvent*  event) override;

    // =========================================================================
    // FUNCTION:    resizeEvent  (override)
    //
    // DESCRIPTION: Repositions the four mode buttons to maintain layout within
    //              the top mode bar after a widget resize. REQ-AESA-004.
    // =========================================================================
    void resizeEvent    (QResizeEvent* event) override;

    // =========================================================================
    // FUNCTION:    mousePressEvent  (override)
    //
    // DESCRIPTION: Left-click handler. Hit-tests all screenTargets within
    //              AESA_HIT pixel radius. If a target is found: sets
    //              lockedTargetID, calls aesa->lockOn(), applies FireControl
    //              mode. If no target found: breaks any existing lock, returns
    //              to Search mode. REQ-AESA-003.
    // =========================================================================
    void mousePressEvent(QMouseEvent*  event) override;

private slots:

    // =========================================================================
    // SLOT:    toggleDisplayMode
    //
    // DESCRIPTION: Cycles displayMode between AIR and SURFACE on each call.
    //              Connected to btnDispMode->clicked. REQ-AESA-004.
    // =========================================================================
    void toggleDisplayMode();

    // =========================================================================
    // SLOT:    onModeButtonClicked
    //
    // DESCRIPTION: Handles SURV (0), TWS (1), LOCK (2) button clicks.
    //              idx=0/1: break lock if active, clear lockedTargetID,
    //              call applyAESAMode().
    //              idx=2: if lockedTargetID == 0, flash "LOCK?" for 800 ms
    //              and return; otherwise call aesa->lockOn() + applyAESAMode.
    //              REQ-AESA-003.
    //
    // PARAMETERS:
    //   modeIndex  — 0 = SURV, 1 = TWS, 2 = LOCK.
    // =========================================================================
    void onModeButtonClicked(int modeIndex);

    // =========================================================================
    // SLOT:    onSensorSelected
    //
    // DESCRIPTION: Called when the operator selects a different AESA sensor
    //              from the sensorDropdown combo box. Rebinds this->sensor to
    //              the sensor at the given index in sensorlist. Resets
    //              lockedTargetID, lockedTargetPos, iffMap_, and
    //              drfmWarnFrames_ so stale state from the previous sensor does
    //              not appear on the new sensor's display. Reconnects the
    //              iffResult, drfmGhostDetected, and schedulerDutyCycle signals
    //              using Qt::UniqueConnection. Re-reads scanMinAz / scanMaxAz
    //              from the new sensor's RadarConfig and auto-selects display
    //              mode (SURFACE if FoV >= 180°, AIR otherwise). Calls
    //              updateModeButtonStyles() and triggers repaint.
    //
    // REQUIREMENT: REQ-AESA-003, REQ-AESA-004
    //
    // PARAMETERS:
    //   index  [in]  Zero-based index into sensorlist of the selected sensor.
    //                Out-of-range values are silently ignored.
    // =========================================================================
    void onSensorSelected(int index);

    // =========================================================================
    // SLOT:    updateDropdown
    //
    // DESCRIPTION: Rebuilds the sensorDropdown combo box contents from the
    //              current sensorlist. Clears any existing items, then adds one
    //              entry per sensor using Sensor::Name; if the name is empty
    //              the fallback label "AESA N" (1-based) is used instead.
    //              Sets the combo box current index to match this->sensor.
    //              Shows the dropdown only when sensorlist contains two or more
    //              sensors; hides it for zero or one sensor so it never appears
    //              on single-sensor platforms. Uses QSignalBlocker to suppress
    //              spurious currentIndexChanged signals during rebuild.
    //              Geometry is managed by repositionButtons() — not here.
    //
    // REQUIREMENT: REQ-AESA-004
    //
    // SIDE EFFECTS: Modifies sensorDropdown item list and visibility.
    //               No sensor or display state is modified.
    // =========================================================================
    void updateDropdown();


private:

    // =========================================================================
    // MODE BUTTONS
    // =========================================================================

    // SURV button. Active QPushButton style (S_ACTIVE) when sensor is in Search.
    QPushButton* btnSurv     = nullptr;

    // TWS button. Active style when sensor is in TrackWhileScan. REQ-AESA-004.
    QPushButton* btnTWS      = nullptr;

    // LOCK button. Lock-on style (S_LOCK_ON) when in FireControl mode.
    // Displays "LOCK?" for 800 ms if LOCK is pressed without a selected target.
    QPushButton* btnLock     = nullptr;

    // Display mode toggle button. Text alternates between "AIR" and "SURF".
    // REQ-AESA-004.
    QPushButton* btnDispMode = nullptr;

    // =========================================================================
    // WIDGET: sensorDropdown
    //
    // DESCRIPTION: Drop-down combo box that lists all AESA sensors attached to
    //              the currently bound platform entity. Visible only when the
    //              platform has two or more AESA sensors (sensorlist.size() > 1);
    //              hidden otherwise to avoid clutter for single-sensor platforms.
    //              Occupies the fifth slot in the top mode bar, positioned and
    //              sized by repositionButtons() on construction and every
    //              resizeEvent() call to maintain alignment with the four mode
    //              buttons. Selecting an item triggers onSensorSelected(), which
    //              rebinds the display to the chosen sensor and resets all lock,
    //              IFF, and DRFM state for the new sensor context.
    //
    // REQUIREMENT: REQ-AESA-004
    // =========================================================================
    QComboBox*   sensorDropdown = nullptr;
    // =========================================================================
    // DISPLAY STATE
    // =========================================================================

    // Current display geometry mode. Determines origin position and ring layout.
    DisplayMode displayMode = DisplayMode::AIR;

    // Left azimuth boundary of scan sector (degrees, body frame).
    // Initialised to -60°. Updated from RadarConfig each updateRadar() call.
    float scanMinAz = -60.0f;

    // Right azimuth boundary of scan sector (degrees, body frame).
    // Initialised to +60°. REQ-AESA-004.
    float scanMaxAz =  60.0f;

    // =========================================================================
    // LOCK STATE
    // =========================================================================

    // Radar ID of the currently locked target. 0 = no target locked.
    // Set by mousePressEvent() and onModeButtonClicked(). REQ-AESA-003.
    uint32_t lockedTargetID  = 0;

    // Widget-space screen position of the locked target's symbol centre.
    // Updated in drawAirTargets() / drawSurfaceTargets() on each paintEvent.
    // Used by drawLockReticle() to place the four-corner bracket. REQ-AESA-003.
    QPointF  lockedTargetPos = {};

    // =========================================================================
    // AESA STATUS (populated from signals each frame)
    // =========================================================================

    // Latest T/R duty cycle [0.0, 1.0]. Populated by schedulerDutyCycle signal.
    // Displayed by drawDutyCycleBar(). FIX-08. REQ-AESA-020.
    float    currentDutyCycle_   = 0.0f;

    // Warning text for the most recent DRFM ghost detection. Set on receipt of
    // drfmGhostDetected signal. Format: "DRFM GHOST  ID:<tid>". REQ-AESA-060.
    QString  lastDRFMWarning_    = "";

    // Radar ID of the most recently flagged DRFM ghost target. Used to colour
    // the corresponding ScreenTarget amber in drawAirTargets(). REQ-AESA-060.
    uint32_t lastDRFMTargetID_   = 0;

    // Frame countdown for DRFM warning display. Set to 60 on drfmGhostDetected.
    // Decremented each updateRadar() call. Warning fades as alpha drops to 0.
    // REQ-AESA-060.
    int      drfmWarnFrames_     = 0;

    // =========================================================================
    // STRUCT: IFFEntry
    //
    // DESCRIPTION: Cached IFF interrogation result for one track. Populated
    //              by iffResult signal from AESARadar. Stored in iffMap_ keyed
    //              by track ID. REQ-AESA-050.
    // =========================================================================
    struct IFFEntry
    {
        // IFF response code — maps to aesa::IFFResponseCode:
        //   0=NO_REPLY  1=FRIENDLY  2=UNKNOWN  3=HOSTILE  4=CORRUPTED
        int      responseCode = 0;

        // Squawk code received from target's IFF transponder. 0 = no reply.
        uint32_t squawk       = 0;

        // Classification confidence [0.0, 1.0]. REQ-AESA-050.
        float    confidence   = 0.0f;
    };

    // Latest IFF result per track ID. Populated by iffResult signal.
    // Read by drawAirTargets() and drawHUD() for symbol colouring and data.
    // REQ-AESA-050.
    QMap<uint32_t, IFFEntry> iffMap_;

    // =========================================================================
    // STRUCT: ScreenTarget
    //
    // DESCRIPTION: One rendered target entry on the current frame. Rebuilt in
    //              drawAirTargets() and drawSurfaceTargets() on every paintEvent.
    //              Used by mousePressEvent() for nearest-target hit-testing.
    //              REQ-AESA-004.
    // =========================================================================
    struct ScreenTarget
    {
        // Widget-space position of the target symbol centre (pixels).
        QPointF  pos;

        // Radar ID resolved via FNV-1a hash of platform->ID (same as
        // platformToRadarID in aesaradar.cpp). 0 if no entity attached.
        uint32_t id;

        // Sensor::Target data (radius, angle, speed, altitude, etc.).
        Target   data;

        // IFF response code from iffMap_. 0 = no data available.
        int      iffCode    = 0;

        // IFF classification confidence [0.0, 1.0].
        float    iffConf    = 0.0f;

        // true = target was flagged as DRFM ghost within the current warning
        // window (drfmWarnFrames_ > 0 and tid == lastDRFMTargetID_).
        // Symbol rendered amber (C_DRFM). REQ-AESA-060.
        bool     isDRFM     = false;

        // Track quality score [0.0, 1.0]. Shown as 5-char bar "| ## .|" in label.
        // Currently always 0.0 — TrackOutput quality not propagated to Target.
        double   trackQual  = 0.0;
    };

    // Per-frame list of rendered target entries. Cleared and rebuilt on every
    // paintEvent call. REQ-AESA-004.
    QVector<ScreenTarget> screenTargets;

    // =========================================================================
    // SCENE GRAPH REFERENCES
    // =========================================================================

    // String ID of the currently bound entity. Compared in RemoveEntity().
    QString    id        = "";

    // Engine Hierarchy pointer. Set by setHierarchy(). May be null. REQ-AESA-004.
    Hierarchy* hierarchy = nullptr;

    // Fallback target list when sensor is null. Unused in normal operation.
    QVector<Target> targets;

    // =========================================================================
    // PRIVATE HELPERS
    // =========================================================================

    // =========================================================================
    // FUNCTION:    asAESA
    //
    // DESCRIPTION: Safe dynamic_cast of sensor to AESARadar*. Returns null if
    //              sensor is null or is not an AESARadar instance.
    //              Used by all methods requiring AESA-specific API. REQ-AESA-003.
    // =========================================================================
    AESARadar* asAESA() const
    {
        return sensor ? dynamic_cast<AESARadar*>(sensor) : nullptr;
    }

    // =========================================================================
    // FUNCTION:    applyAESAMode
    //
    // DESCRIPTION: Translates Sensor::Mode to aesa::RadarMode and applies it
    //              via AESARadar::setRadarConfig(). Syncs sensor->mode.
    //              Calls markDisplayRangeDirty(). REQ-AESA-003.
    //
    // PARAMETERS:
    //   m  [in]  Target sensor mode (Search / TrackWhileScan / FireControl).
    // =========================================================================
    void applyAESAMode(Sensor::Mode m);

    // =========================================================================
    // FUNCTION:    updateModeButtonStyles
    //
    // DESCRIPTION: Applies correct QPushButton stylesheet to SURV/TWS/LOCK
    //              based on current sensor->mode. If sensor is null, all get
    //              S_IDLE. REQ-AESA-003.
    // =========================================================================
    void updateModeButtonStyles();

    // =========================================================================
    // FUNCTION:    repositionButtons
    //
    // DESCRIPTION: Lays out all four mode buttons and the sensorDropdown in the
    //              top mode bar at equal widths within [AESA_MARGIN,
    //              width - AESA_MARGIN]. The available bar width is divided into
    //              five equal slots: SURV, TWS, LOCK, AIR/SURF, and the sensor
    //              selector dropdown. Called on construction and every
    //              resizeEvent() to maintain correct layout after widget resize.
    //              The dropdown geometry is set here so it always stays aligned
    //              with the buttons regardless of widget width. REQ-AESA-004.
    // =========================================================================
    void repositionButtons();

    // =========================================================================
    // DRAWING HELPERS — pure render functions, no state modification
    // =========================================================================

    // Fills black background. Draws top mode bar and main display rect borders.
    void drawBackground    (QPainter& p);

    // Draws small grey filled square at (cx, cy) — radar origin marker.
    void drawCenterMark    (QPainter& p, int cx, int cy);

    // Renders HUD: range, azimuth, beamWidth, mode string, duty %, track count,
    // platform name, lock status, IFF of locked target, IFF colour legend.
    void drawHUD           (QPainter& p);

    // Draws four-corner lock reticle at lockedTargetPos when lockedTargetID != 0.
    void drawLockReticle   (QPainter& p);

    // Draws DRFM ghost warning text (amber, alpha-faded) when drfmWarnFrames_ > 0.
    void drawDRFMWarning   (QPainter& p);

    // Draws duty cycle bar bottom-right: green < 60%, amber < 85%, red >= 85%.
    void drawDutyCycleBar  (QPainter& p);

    // Draws AIR-mode range rings (dashed) with km labels. Ring step: 5/10/20 km.
    void drawAirRings      (QPainter& p, int cx, int cy);

    // Draws dashed scan sector boundary lines at scanMinAz and scanMaxAz.
    void drawAirSector     (QPainter& p, int cx, int cy);

    // Renders targets in AIR mode. Rebuilds screenTargets. IFF colour applied.
    // DRFM amber applied. TWS coasted tracks shown outside sector. SURV filtered.
    void drawAirTargets    (QPainter& p, int cx, int cy);

    // Draws full 360° PPI rings with N/S/E/W cardinal labels.
    void drawSurfaceRings  (QPainter& p, int cx, int cy, int radius);

    // Renders surface targets as squares in 360° PPI mode.
    void drawSurfaceTargets(QPainter& p, int cx, int cy, int radius);

    // Draws a green circle symbol for an air target.
    void drawCircle        (QPainter& p, int x, int y, int sz, QColor col);

    // Draws a filled square symbol for a surface target.
    void drawSquare        (QPainter& p, int x, int y, int sz, QColor col);

    // Draws data label beside a target: speed, altitude, heading, radial
    // velocity, IFF badge, DRFM tag, track quality bar, heading vector line.
    void drawLabel         (QPainter& p, int x, int y,
                   const ScreenTarget& st, bool isLocked);

    // =========================================================================
    // FUNCTION:    iffColour
    //
    // DESCRIPTION: Maps an IFF response code to the display colour used for
    //              target symbols and IFF badge labels.
    //                1 (FRIENDLY) → C_FRIENDLY (cyan)
    //                3 (HOSTILE)  → C_HOSTILE  (red)
    //                2 (UNKNOWN)  → C_UNKNOWN  (yellow)
    //                other        → C_AIR      (green)
    //
    // PARAMETERS:
    //   responseCode  [in]  aesa::IFFResponseCode cast to int.
    //
    // RETURNS:    QColor for rendering. REQ-AESA-050.
    // SIDE EFFECTS: None. Pure query.
    // =========================================================================
    QColor iffColour(int responseCode) const;
};

#endif // AESARADARDISPLAY_H



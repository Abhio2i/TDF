// =============================================================================
// FILE:        frame.h
// MODULE:      Performance Frame Analysis
// PROJECT:     Tactical Display/Simulation Framework (TDF)
// ORGANISATION: Oxygen 2 Innovation (O2I)
// STANDARD:    RTCA DO-178C / ED-12C, DAL B (Guidelines applied)
//
// DESCRIPTION: Defines the ExcutesLog struct and Frame class for tracking
//              execution times of various simulation subsystems per frame.
//              Stores timing data for GUI, canvas, physics, dynamics, sensors,
//              radar, EW, CSM, ESM, IFF, radio, and entity counts. Provides
//              methods to clear and analyze frame performance.
//
// AUTHOR:      [Original Author Name]
// REVIEWED BY: [Reviewer Name], [Review Date]
//
// CHANGE HISTORY:
//   Rev 1  [Date]  Initial implementation.
//
// COPYRIGHT:   Oxygen 2 Innovation (O2I). All rights reserved.
// =============================================================================

#ifndef FRAME_H
#define FRAME_H

#include <QObject>
using namespace std;

// =============================================================================
// STRUCT: ExcutesLog
// DESCRIPTION: Records execution timing for a single subsystem or operation.
// =============================================================================
struct ExcutesLog{
    int number;             //!< Sequential number of the execution
    string startTime;       //!< Start timestamp (format unspecified)
    string endTime;         //!< End timestamp
    string excutionTime;    //!< Duration as string
};

// =============================================================================
// CLASS: Frame
//
// DESCRIPTION: Holds performance metrics for one simulation frame. Tracks
//              execution times (in milliseconds) for various subsystems and
//              entity counts. Provides methods to reset data and perform
//              analysis.
// =============================================================================
class Frame: public QObject
{
    Q_OBJECT
public:
    Frame();

    // =========================================================================
    // SECTION: Execution Logs
    // DESCRIPTION: Map of subsystem names to their execution logs.
    // =========================================================================
    std::unordered_map<std::string, ExcutesLog> *excutesLogs;  //!< Per‑subsystem logs

    // =========================================================================
    // SECTION: Frame Timing
    // DESCRIPTION: Overall frame timing and per‑subsystem breakdown (ms).
    // =========================================================================
    int number;                 //!< Frame number (sequential)
    string startTime;           //!< Frame start timestamp
    string endTime;             //!< Frame end timestamp
    int excutionTime = 0;       //!< Total frame execution time (ms)
    int GUITime = 0;            //!< GUI update time (ms)
    int canvasTime = 0;         //!< Canvas/rendering time (ms)
    int physicsTime = 0;        //!< Physics simulation time (ms)
    int dynamicTime = 0;        //!< Dynamic model update time (ms)
    int SensorTime = 0;         //!< Sensor processing time (ms)
    int RadarTime = 0;          //!< Radar specific time (ms)
    int EWTime = 0;             //!< Electronic Warfare time (ms)
    int CSMTime = 0;            //!< CSM (Communication System Module?) time (ms)
    int ESMTime = 0;            //!< ESM (Electronic Support Measures) time (ms)
    int IFFTime = 0;            //!< IFF processing time (ms)
    int RadioTime = 0;          //!< Radio communication time (ms)

    // =========================================================================
    // SECTION: Entity Counts
    // DESCRIPTION: Number of active simulation objects per frame.
    // =========================================================================
    int totalEntity = 0;        //!< Total entities in scene
    int totalSensor = 0;        //!< Number of sensors
    int totalRadio = 0;         //!< Number of radios
    int totalIff = 0;           //!< Number of IFF components
    int csmdisplay = 0;         //!< CSM display count (purpose specific)
    int totaldynamicModel = 0;  //!< Number of dynamic models
    int esmdisplay = 0;         //!< ESM display count

    void clean();               //!< Resets all timing and count fields to zero
    void analyze();             //!< Performs analysis on collected frame data
};

#endif // FRAME_H

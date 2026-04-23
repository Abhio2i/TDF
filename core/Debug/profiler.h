// =============================================================================
// FILE:        profiler.h
// MODULE:      Performance Profiling
// PROJECT:     Tactical Display/Simulation Framework (TDF)
// ORGANISATION: Oxygen 2 Innovation (O2I)
// STANDARD:    RTCA DO-178C / ED-12C, DAL B (Guidelines applied)
//
// DESCRIPTION: Defines the Profiler class, which manages performance profiling
//              data across multiple frames. Maintains a static pointer to the
//              current frame and a collection of historical frames. Provides
//              methods to add execution logs for subsystems.
//
// AUTHOR:      [Original Author Name]
// REVIEWED BY: [Reviewer Name], [Review Date]
//
// CHANGE HISTORY:
//   Rev 1  [Date]  Initial implementation.
//
// COPYRIGHT:   Oxygen 2 Innovation (O2I). All rights reserved.
// =============================================================================

#ifndef PROFILER_H
#define PROFILER_H

#include <QObject>
#include <core/Debug/frame.h>

// =============================================================================
// CLASS: Profiler
//
// DESCRIPTION: Singleton-like profiler that tracks frame-by-frame performance
//              metrics. Holds a map of Frame objects keyed by frame identifier
//              (e.g., timestamp or frame number). The static currentFrame
//              points to the most recently recorded frame for easy access.
// =============================================================================
class Profiler: public QObject
{
    Q_OBJECT
public:
    Profiler();
    ~Profiler();

    static Frame *currentFrame;                         //!< Pointer to the current frame being profiled
    std::unordered_map<std::string, Frame> *frames;     //!< Map of frame identifier to Frame data

    void addExecuteLogs();                              //!< Adds execution logs for the current frame
};

#endif // PROFILER_H

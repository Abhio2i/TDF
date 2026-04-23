// =============================================================================
// FILE:        console.h
// MODULE:      Console Logging System
// PROJECT:     Tactical Display/Simulation Framework (TDF)
// ORGANISATION: Oxygen 2 Innovation (O2I)
// STANDARD:    RTCA DO-178C / ED-12C, DAL B (Guidelines applied)
//
// DESCRIPTION: Defines the Console class, a singleton providing logging
//              facilities for the simulation framework. Supports log, error,
//              and warning messages with signals for UI updates. Stores logs
//              in an unordered map and provides save, get, and clear operations.
//
// AUTHOR:      [Original Author Name]
// REVIEWED BY: [Reviewer Name], [Review Date]
//
// CHANGE HISTORY:
//   Rev 1  [Date]  Initial implementation.
//
// COPYRIGHT:   Oxygen 2 Innovation (O2I). All rights reserved.
// =============================================================================

#ifndef CONSOLE_H
#define CONSOLE_H

#include <QObject>
#include <unordered_map>

// =============================================================================
// CLASS: Console
//
// DESCRIPTION: Singleton logging utility. Collects log messages, errors, and
//              warnings in a map (key-value pairs). Emits signals for each
//              message type to notify UI components. Provides static methods
//              for easy logging from anywhere in the codebase.
// =============================================================================
class Console: public QObject
{
    Q_OBJECT
public:
    static Console* internalInstance();                     //!< Returns the singleton instance

    std::unordered_map<std::string, std::string> *logList;  //!< Map of log entries (ID -> message)

    static void log(std::string log);                       //!< Logs an informational message
    static void log(QJsonObject obj);                       //!< Logs a JSON object (converted to string)
    static void error(std::string error);                   //!< Logs an error message
    static void warning(std::string warning);               //!< Logs a warning message

    void saveLog();     //!< Persists the current log to disk
    void getLog();      //!< Retrieves logs from storage (implementation specific)
    void clearLog();    //!< Clears all log entries

signals:
    void logUpdate(std::string log);        //!< Emitted when a log message is added
    void errorUpdate(std::string error);    //!< Emitted when an error is logged
    void warningUpdate(std::string warning);//!< Emitted when a warning is logged
    void debugUpdate(std::string debug);    //!< Emitted for debug messages

private:
    Console();          //!< Private constructor (singleton pattern)
};

#endif // CONSOLE_H

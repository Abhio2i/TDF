// =============================================================================
// FILE:        sessionmanager.h
// MODULE:      Session Preferences Management
// PROJECT:     Tactical Display/Simulation Framework (TDF)
// ORGANISATION: Oxygen 2 Innovation (O2I)
// STANDARD:    RTCA DO-178C / ED-12C, DAL B (Guidelines applied)
//
// DESCRIPTION: Defines the SessionManager class, which manages user session
//              preferences and temporary preferences as variant maps
//              (int, float, double, string, bool, char). Provides methods
//              for cleaning, retrieving, setting, and checking preferences,
//              along with JSON serialisation for persistence.
//
// AUTHOR:      [Original Author Name]
// REVIEWED BY: [Reviewer Name], [Review Date]
//
// CHANGE HISTORY:
//   Rev 1  [Date]  Initial implementation.
//
// COPYRIGHT:   Oxygen 2 Innovation (O2I). All rights reserved.
// =============================================================================

#ifndef SESSIONMANAGER_H
#define SESSIONMANAGER_H

#include <QObject>
#include <variant>

// =============================================================================
// CLASS: SessionManager
//
// DESCRIPTION: Handles user session preferences and temporary preferences
//              using string keys and variant values. Supports multiple data
//              types and JSON serialisation for saving/loading session state.
// =============================================================================
class SessionManager: public QObject
{
    Q_OBJECT
public:
    SessionManager();

    // =========================================================================
    // SECTION: Preference Maps
    // DESCRIPTION: Persistent and temporary key-value stores.
    // =========================================================================
    std::unordered_map<std::string, std::variant<int, float, double, std::string, bool, char>> *prefs;      //!< Persistent preferences
    std::unordered_map<std::string, std::variant<int, float, double, std::string, bool, char>> *tempprefs; //!< Temporary session preferences

    // Preference operations
    void cleanPrefs();      //!< Clears all preferences
    void getPrefs();        //!< Retrieves preference values
    void setPrefs();        //!< Sets preference values
    void isPrefs();         //!< Checks if preferences exist
    void getPrefType();     //!< Gets the type of a preference value

    // Serialization
    void toJson();          //!< Serialises session manager to JSON
    void fromJson();        //!< Deserialises session manager from JSON
};

#endif // SESSIONMANAGER_H

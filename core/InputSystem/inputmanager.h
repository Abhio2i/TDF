// =============================================================================
// FILE:        inputmanager.h
// MODULE:      Input Management
// PROJECT:     Tactical Display/Simulation Framework (TDF)
// ORGANISATION: Oxygen 2 Innovation (O2I)
// STANDARD:    RTCA DO-178C / ED-12C, DAL B (Guidelines applied)
//
// DESCRIPTION: Defines the InputManager class (singleton) which captures
//              keyboard and mouse input events, tracks pressed keys/buttons,
//              mouse position, and provides static methods for aircraft
//              control inputs (throttle, pitch, roll, yaw, air brakes).
//
// AUTHOR:      [Original Author Name]
// REVIEWED BY: [Reviewer Name], [Review Date]
//
// CHANGE HISTORY:
//   Rev 1  [Date]  Initial implementation.
//
// COPYRIGHT:   Oxygen 2 Innovation (O2I). All rights reserved.
// =============================================================================

#ifndef INPUTMANAGER_H
#define INPUTMANAGER_H

#include <QObject>
#include <QSet>
#include <QPoint>
#include <QKeyEvent>
#include <QMouseEvent>

// =============================================================================
// CLASS: InputManager
//
// DESCRIPTION: Singleton that provides centralised input handling. Filters
//              events to track keyboard and mouse state. Offers query methods
//              for key/mouse states and specialised static methods for
//              aircraft control axes (throttle, pitch, roll, yaw) and air brakes.
// =============================================================================
class InputManager : public QObject
{
    Q_OBJECT

public:
    static InputManager* instance();                    //!< Returns singleton instance

    bool isKeyPressed(Qt::Key key) const;               //!< Checks if a specific key is pressed
    bool isMouseButtonPressed(Qt::MouseButton button) const; //!< Checks if a mouse button is pressed
    QPoint getMousePosition() const;                    //!< Returns current mouse cursor position

    // =========================================================================
    // SECTION: Aircraft Control Inputs
    // DESCRIPTION: Static methods for reading flight control axes.
    // =========================================================================
    static float getThrottleInput();    //!< Returns throttle value (0..1)
    static float getPitchInput();       //!< Returns pitch axis (-1..1)
    static float getRollInput();        //!< Returns roll axis (-1..1)
    static float getYawInput();         //!< Returns yaw axis (-1..1)
    static bool getAirBrakes();         //!< Returns air brake state (true = braking)

protected:
    bool eventFilter(QObject *obj, QEvent *event) override; //!< Event filter to capture inputs

private:
    explicit InputManager(QObject *parent = nullptr);       //!< Private constructor (singleton)
    static InputManager* m_instance;                        //!< Singleton instance pointer

    QSet<Qt::Key> m_pressedKeys;            //!< Set of currently pressed keys
    QSet<Qt::MouseButton> m_pressedMouseButtons; //!< Set of pressed mouse buttons
    QPoint m_mousePosition;                 //!< Last known mouse cursor position
};

#define INPUT InputManager::instance()      //!< Convenience macro for quick access

#endif // INPUTMANAGER_H

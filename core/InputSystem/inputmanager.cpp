 #include "inputmanager.h"
#include <QApplication> // Add this include
#include <QMouseEvent>  // Ensure this is included for QMouseEvent

InputManager* InputManager::m_instance = nullptr;

InputManager::InputManager(QObject *parent) : QObject(parent)
{
}

InputManager* InputManager::instance()
{
    if (!m_instance)
    {
        m_instance = new InputManager();
        qApp->installEventFilter(m_instance);
    }
    return m_instance;
}

bool InputManager::isKeyPressed(Qt::Key key) const
{
    return m_pressedKeys.contains(key);
}

bool InputManager::isMouseButtonPressed(Qt::MouseButton button) const
{
    return m_pressedMouseButtons.contains(button);
}

QPoint InputManager::getMousePosition() const
{
    return m_mousePosition;
}

bool InputManager::eventFilter(QObject *obj, QEvent *event)
{
    switch (event->type())
    {
    case QEvent::KeyPress:
        m_pressedKeys.insert(static_cast<Qt::Key>(static_cast<QKeyEvent *>(event)->key()));

        break;

    case QEvent::KeyRelease:
        m_pressedKeys.remove(static_cast<Qt::Key>(static_cast<QKeyEvent *>(event)->key()));
        break;

    case QEvent::MouseButtonPress:
        m_pressedMouseButtons.insert(static_cast<QMouseEvent *>(event)->button());
        break;

    case QEvent::MouseButtonRelease:
        m_pressedMouseButtons.remove(static_cast<QMouseEvent *>(event)->button());
        break;

    case QEvent::MouseMove:
        m_mousePosition = static_cast<QMouseEvent *>(event)->globalPos();
        break;

    default:
        break;
    }

    return QObject::eventFilter(obj, event);
}

// New implementations for aircraft control inputs
float InputManager::getThrottleInput() {
    if (instance()->isKeyPressed(Qt::Key_W)) {
        return 1.0f;
    }
    return 0.0f;
}

float InputManager::getPitchInput() {
    if (instance()->isKeyPressed(Qt::Key_K)) { // Key for Pitch Up
        return 1.0f;
    }
    if (instance()->isKeyPressed(Qt::Key_I)) { // Key for Pitch Down
        return -1.0f;
    }
    return 0.0f;
}

float InputManager::getRollInput() {
    if (instance()->isKeyPressed(Qt::Key_D)) { // Key for Roll Right
        return 1.0f;
    }
    if (instance()->isKeyPressed(Qt::Key_A)) { // Key for Roll Left
        return -1.0f;
    }
    return 0.0f;
}

float InputManager::getYawInput() {
    if (instance()->isKeyPressed(Qt::Key_L)) { // Key for Yaw Right
        return 1.0f;
    }
    if (instance()->isKeyPressed(Qt::Key_J)) { // Key for Yaw Left
        return -1.0f;
    }
    return 0.0f;
}

bool InputManager::getAirBrakes() {
    return instance()->isKeyPressed(Qt::Key_S);
}

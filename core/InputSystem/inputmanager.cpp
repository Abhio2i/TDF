#include "InputManager.h"

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
        m_mousePosition = static_cast<QMouseEvent *>(event)->globalPosition().toPoint();
        break;

    default:
        break;
    }

    return QObject::eventFilter(obj, event);
}

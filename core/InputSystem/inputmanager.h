#ifndef INPUTMANAGER_H
#define INPUTMANAGER_H

#include <QObject>
#include <QSet>
#include <QPoint>
#include <QKeyEvent>
#include <QMouseEvent>

class InputManager : public QObject
{
    Q_OBJECT

public:
    static InputManager* instance();

    bool isKeyPressed(Qt::Key key) const;
    bool isMouseButtonPressed(Qt::MouseButton button) const;
    QPoint getMousePosition() const;
    // New static methods for aircraft control inputs
    static float getThrottleInput();
    static float getPitchInput();
    static float getRollInput();
    static float getYawInput();
    static bool getAirBrakes();

protected:
    bool eventFilter(QObject *obj, QEvent *event) override;

private:
    explicit InputManager(QObject *parent = nullptr);
    static InputManager* m_instance;

    QSet<Qt::Key> m_pressedKeys;
    QSet<Qt::MouseButton> m_pressedMouseButtons;
    QPoint m_mousePosition;
};
#define INPUT InputManager::instance()
#endif // INPUTMANAGER_H

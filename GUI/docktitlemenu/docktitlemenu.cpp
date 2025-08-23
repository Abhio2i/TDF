
#include "docktitlemenu.h"
#include <QEvent>
#include <QCursor>

DockTitleMenu::DockTitleMenu(QDockWidget *parentDock, QObject *parent)
    : QObject(parent), m_dock(parentDock)
{
    m_menu = new QMenu(m_dock);
}

void DockTitleMenu::setupMenu(QPushButton *menuButton)
{
    QAction *lockAction = m_menu->addAction("Lock/Unlock");
    QAction *copyAction = m_menu->addAction("Copy");
    QAction *pasteAction = m_menu->addAction("Paste");
    QAction *inspectorAction = m_menu->addAction("Add Inspector Tab");

    connect(lockAction, &QAction::triggered, this, [this](bool) {
        bool isLocked = !m_dock->features().testFlag(QDockWidget::DockWidgetMovable);
        emit lockRequested(isLocked);
    });

    connect(copyAction, &QAction::triggered, this, &DockTitleMenu::copyRequested);
    connect(pasteAction, &QAction::triggered, this, &DockTitleMenu::pasteRequested);
    connect(inspectorAction, &QAction::triggered, this, &DockTitleMenu::addInspectorRequested);

    connect(menuButton, &QPushButton::clicked, [this, menuButton](bool) {
        m_menu->exec(menuButton->mapToGlobal(QPoint(0, menuButton->height())));
    });
}

HoverEventFilter::HoverEventFilter(QPushButton *button, QObject *parent)
    : QObject(parent), m_button(button)
{
}

bool HoverEventFilter::eventFilter(QObject *obj, QEvent *event)
{
    if (event->type() == QEvent::HoverEnter) {
        m_button->setVisible(true);
    } else if (event->type() == QEvent::HoverLeave) {
        QPoint pos = QCursor::pos();
        QWidget *widget = qobject_cast<QWidget*>(obj);
        if (widget && !widget->rect().contains(widget->mapFromGlobal(pos))) {
            m_button->setVisible(false);
        }
    }
    return QObject::eventFilter(obj, event);
}

/* ========================================================================= */
/* File: docktitlemenu.cpp                                                  */
/* Purpose: Implements menu and hover event filter for dock widget title     */
/* ========================================================================= */

#include "docktitlemenu.h"                        // For dock title menu classes
#include <QEvent>                                 // For event handling
#include <QCursor>                                // For cursor position

// %%% DockTitleMenu Implementation %%%
/* Constructor for dock title menu */
DockTitleMenu::DockTitleMenu(QDockWidget *parentDock, QObject *parent)
    : QObject(parent), m_dock(parentDock)
{
    // Initialize menu
    m_menu = new QMenu(m_dock);
}

/* Setup menu with actions */
void DockTitleMenu::setupMenu(QPushButton *menuButton)
{
    // Add lock/unlock action
    QAction *lockAction = m_menu->addAction("Lock/Unlock");
    // Add copy action
    QAction *copyAction = m_menu->addAction("Copy");
    // Add paste action
    QAction *pasteAction = m_menu->addAction("Paste");
    // Add inspector tab action
    QAction *inspectorAction = m_menu->addAction("Add Inspector Tab");

    // Connect lock action
    connect(lockAction, &QAction::triggered, this, [this](bool) {
        // Check if dock is movable
        bool isLocked = !m_dock->features().testFlag(QDockWidget::DockWidgetMovable);
        // Emit lock request signal
        emit lockRequested(isLocked);
    });

    // Connect copy action
    connect(copyAction, &QAction::triggered, this, &DockTitleMenu::copyRequested);
    // Connect paste action
    connect(pasteAction, &QAction::triggered, this, &DockTitleMenu::pasteRequested);
    // Connect inspector action
    connect(inspectorAction, &QAction::triggered, this, &DockTitleMenu::addInspectorRequested);

    // Connect menu button click
    connect(menuButton, &QPushButton::clicked, [this, menuButton](bool) {
        // Show menu at button position
        m_menu->exec(menuButton->mapToGlobal(QPoint(0, menuButton->height())));
    });
}

// %%% HoverEventFilter Implementation %%%
/* Constructor for hover event filter */
HoverEventFilter::HoverEventFilter(QPushButton *button, QObject *parent)
    : QObject(parent), m_button(button)
{
}

/* Handle hover events */
bool HoverEventFilter::eventFilter(QObject *obj, QEvent *event)
{
    // Check for hover enter event
    if (event->type() == QEvent::HoverEnter) {
        // Show button
        m_button->setVisible(true);
        // Check for hover leave event
    } else if (event->type() == QEvent::HoverLeave) {
        // Get cursor position
        QPoint pos = QCursor::pos();
        // Cast object to widget
        QWidget *widget = qobject_cast<QWidget*>(obj);
        // Hide button if cursor outside widget
        if (widget && !widget->rect().contains(widget->mapFromGlobal(pos))) {
            m_button->setVisible(false);
        }
    }
    // Pass event to base class
    return QObject::eventFilter(obj, event);
}

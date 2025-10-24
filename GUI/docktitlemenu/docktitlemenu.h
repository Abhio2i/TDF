//============================================================================
// File        : docktitlemenu.h
// Description : Header file for DockTitleMenu class that provides a menu
//               for QDockWidget title bars with actions like lock, copy, paste,
//               and add inspector. Includes HoverEventFilter for button hover.
//============================================================================

#ifndef DOCKTITLEMENU_H
#define DOCKTITLEMENU_H

#include <QObject>
#include <QPushButton>
#include <QDockWidget>
#include <QMenu>

//============================================================================
// CLASS: DockTitleMenu
//============================================================================

/**
 * @brief DockTitleMenu provides a context menu for a QDockWidget title bar.
 *
 * Features:
 *   - Lock/unlock the dock
 *   - Copy and paste dock content
 *   - Add inspector panel
 *
 * Emits signals when menu actions are triggered.
 */
class DockTitleMenu : public QObject
{
    Q_OBJECT

public:
    /**
     * @brief Constructs DockTitleMenu for a given dock widget.
     * @param parentDock Pointer to the target QDockWidget.
     * @param parent Optional QObject parent.
     */
    explicit DockTitleMenu(QDockWidget *parentDock, QObject *parent = nullptr);

    /**
     * @brief Sets up the menu on the given QPushButton.
     * @param menuButton Pointer to the QPushButton that triggers the menu.
     */
    void setupMenu(QPushButton *menuButton);

signals:
    void lockRequested(bool locked);     ///< Emitted when lock/unlock is requested
    void copyRequested();                ///< Emitted when copy action is triggered
    void pasteRequested();               ///< Emitted when paste action is triggered
    void addInspectorRequested();        ///< Emitted when add inspector is requested

private:
    QDockWidget *m_dock; ///< Target dock widget
    QMenu *m_menu;       ///< Context menu for the dock
};

//============================================================================
// CLASS: HoverEventFilter
//============================================================================

/**
 * @brief HoverEventFilter installs an event filter on a QPushButton to detect hover events.
 *
 * Used to provide visual feedback when the user hovers over menu buttons.
 */
class HoverEventFilter : public QObject
{
    Q_OBJECT

public:
    /**
     * @brief Constructs a HoverEventFilter for a given button.
     * @param button Pointer to the QPushButton to filter hover events.
     * @param parent Optional QObject parent.
     */
    HoverEventFilter(QPushButton *button, QObject *parent = nullptr);

protected:
    /**
     * @brief Overrides eventFilter to capture hover events.
     * @param obj QObject being filtered.
     * @param event Event object to process.
     * @return true if event is handled, false otherwise.
     */
    bool eventFilter(QObject *obj, QEvent *event) override;

private:
    QPushButton *m_button; ///< Button being monitored for hover events
};

#endif // DOCKTITLEMENU_H

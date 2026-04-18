/* ========================================================================= */
/* File: menubar-styles.h                                                   */
/* Purpose: Dark theme styles for MenuBar                                   */
/* Background: #0F2636, Text: White, Hover: #1A3652, Accent: #0078D4        */
/* Written by   : Arti Rajpoot                                               */
/* ========================================================================= */

#ifndef MENUBAR_STYLES_H
#define MENUBAR_STYLES_H

#include <QString>

namespace MenuBarStyles {

/* ============================================================================
   MENUBAR DARK THEME
   Background: #0F2636, Hover: #1A3652, Text: White, Accent: #0078D4
============================================================================ */

/* Main MenuBar */
const QString MenuBar = R"(
    QMenuBar {
        background-color: #0F2636;
        color: white;
        border: none;
        border-bottom: 1px solid #27446d;
        font-size: 12px;
        padding: 2px 0;
    }
    QMenuBar::item {
        background-color: transparent;
        color: white;
        padding: 6px 12px;
        margin: 2px 0;
        border-radius: 2px;
    }
    QMenuBar::item:selected {
        background-color: #1A3652;
        color: white;
    }
    QMenuBar::item:pressed {
        background-color: #27446d;
    }
    QMenuBar::item:disabled {
        color: #666666;
    }
)";

/* Menu (dropdown) */
const QString Menu = R"(
    QMenu {
        background-color: #1A3652;
        color: white;
        border: 1px solid #27446d;
        border-radius: 2px;
        font-size: 12px;
        padding: 4px 0;
    }
    QMenu::item {
        padding: 6px 20px 6px 25px;
        color: white;
        background-color: transparent;
        border: none;
        margin: 1px 4px;
    }
    QMenu::item:selected {
        background-color: #0078D4;
        color: white;
        border-radius: 2px;
    }
    QMenu::item:disabled {
        color: #666666;
        background-color: transparent;
    }
    QMenu::separator {
        height: 1px;
        background-color: #27446d;
        margin: 4px 0;
    }
    /* Optional checkmark for checked items (if needed) */
    QMenu::indicator {
        width: 12px;
        height: 12px;
        margin-left: 5px;
    }

)";

} // namespace MenuBarStyles

#endif // MENUBAR_STYLES_H

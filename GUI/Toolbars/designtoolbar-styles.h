/* ========================================================================= */
/* File: designtoolbar-styles.h                                             */
/* Purpose: Dark theme styles for DesignToolBar - UNIFORM SPACING           */
/* Background: #0F2636, Border: #27446d, Text: White, Accent: #0078D4       */
/* Icon size: 16x16 - COMPACT ACTIVE STATE                                  */
/* ========================================================================= */

#ifndef DESIGNTOOLBAR_STYLES_H
#define DESIGNTOOLBAR_STYLES_H

#include <QString>

namespace DesignToolbarStyles {

/* ============================================================================
   DESIGN TOOLBAR DARK THEME - UNIFORM SPACING
   Background: #0F2636, Border: #27446d, Text: White, Accent: #0078D4
============================================================================ */

/* Main Toolbar - UNIFORM SPACING */
const QString Toolbar = R"(
    QToolBar {
        background-color: #0F2636;
        border: none;
        border-bottom: 1px solid #27446d;
        spacing: 2px;  /* Uniform spacing between all buttons */
        padding: 2px;
    }
    QToolBar::separator {
        background-color: #27446d;
        width: 1px;
        height: 16px;
        margin: 4px 2px;
    }
)";

/* Toolbar Buttons - UNIFORM SIZE */
const QString ToolbarButton = R"(
    QToolButton {
        background-color: transparent;
        color: white;
        border: none;
        border-radius: 2px;
        padding: 0px;  /* No padding - using fixed size instead */
        margin: 0px;
        min-width: 24px;  /* Fixed small size */
        max-width: 24px;
        min-height: 24px;
        max-height: 24px;
    }
    QToolButton:hover {
        background-color: #1A3652;
        border-radius: 2px;
    }
    QToolButton:pressed {
        background-color: #27446d;
    }
    QToolButton:checked {
        background-color: #27446d;
        border-left: 2px solid #0078D4;
    }
    QToolButton:disabled {
        color: #666666;
    }
    QToolButton::menu-indicator {
        image: none;
        width: 0px;
    }
)";

/* Highlighted Button (active action) - COMPACT VERSION */
const QString ToolbarButtonHighlighted = R"(
    QToolButton {
        background-color: #27446d;
        border: 1px solid #0078D4;
        border-radius: 2px;
        padding: 0px;
        margin: 0px;
        min-width: 24px;
        max-width: 24px;
        min-height: 24px;
        max-height: 24px;
    }
)";

/* Menu Button with Popup - SAME SIZE AS REGULAR BUTTONS */
const QString Menu = R"(
    QToolButton {
        background-color: transparent;
        color: white;
        border: none;
        border-radius: 2px;
        padding: 0px;
        margin: 0px;
        min-width: 24px;
        max-width: 24px;
        min-height: 24px;
        max-height: 24px;
    }
    QToolButton:hover {
        background-color: #1A3652;
        border-radius: 2px;
    }
    QToolButton:pressed {
        background-color: #27446d;
    }
    QToolButton:checked {
        background-color: #27446d;
        border-left: 2px solid #0078D4;
    }
    QToolButton::menu-indicator {
        image: none;
        width: 0px;
    }
)";

/* Rest of the styles remain the same... */



/* StayOpenMenu (Custom menu) */
/* StayOpenMenu - BASE64 PNG WHITE CHECKMARK (100% WORKS) */
/* StayOpenMenu - PNG FROM RESOURCES */
const QString StayOpenMenu = R"(
    QMenu {
        background-color: #1A3652;
        color: white;
        border: 1px solid #27446d;
        border-radius: 2px;
        font-size: 11px;
        padding: 4px 0;
    }
    QMenu::item {
        padding: 5px 10px 4px 25px;
        color: white;
        background-color: transparent;
        border: none;
        margin: 1px 2px;
    }
    QMenu::item:selected {
        background-color: #27446d;
        color: white;
    }
    QMenu::item:checked {
        background-color: #1E3E5E;
        color: white;
        font-weight: bold;
    }
    /* CHECKBOX WITH PNG CHECKMARK */
    QMenu::indicator {
        width: 12px;
        height: 12px;
        margin-left: 8px;
        margin-right: 8px;
        subcontrol-position: left center;
        subcontrol-origin: padding;
        border: 2px solid #4A6A8A;
        border-radius: 4px;
        background-color: #0A1A2A;
    }
    QMenu::indicator:checked {

        image: url(:/icons/images/check.png);
    }
    QMenu::indicator:checked:hover {

        image: url(:/icons/images/check.png);
    }
    QMenu::indicator:unchecked {

        image: none;
    }
    QMenu::indicator:unchecked:hover {

        image: none;
    }
    QMenu::separator {
        height: 1px;
        background-color: #27446d;
        margin: 6px 0;
    }
)";

/* Menu Label (section headers) */
const QString MenuLabel = R"(
    QLabel {
        font-weight: bold;
        padding: 5px;
        background-color: #1A3652;
        color: #CCCCCC;
        border-bottom: 1px solid #27446d;
    }
)";

/* Push Button (for add layer buttons) */
const QString PushButton = R"(
    QPushButton {
        background-color: #4CAF50;
        color: white;
        border: none;
        border-radius: 12px;
        font-weight: bold;
        font-size: 14px;
    }
    QPushButton:hover {
        background-color: #45a049;
    }
    QPushButton:disabled {
        background-color: #4A4A4A;
        color: #888888;
    }
)";

/* Line Edit (for search input) */
const QString LineEdit = R"(
    QLineEdit {
        background-color: #1A3652;
        color: white;
        border: 1px solid #27446d;
        border-radius: 2px;
        padding: 5px;
        font-size: 11px;
        min-height: 20px;
    }
    QLineEdit:focus {
        border: 1px solid #0078D4;
        background-color: #1E3E5E;
    }
    QLineEdit::placeholder {
        color: #B0B0B0;
        font-style: italic;
    }
)";

/* Dialog */
const QString Dialog = R"(
    QDialog {
        background-color: #0F2636;
        color: white;
        border: 2px solid #27446d;
        border-radius: 3px;
    }
    QLabel {
        color: white;
        font-size: 11px;
        background-color: transparent;
    }
)";

/* File Dialog */
const QString FileDialog = R"(
    QFileDialog {
        background-color: #0F2636;
        color: white;
    }
    QFileDialog QLabel {
        color: white;
    }
    QFileDialog QLineEdit {
        background-color: #1A3652;
        color: white;
        border: 1px solid #27446d;
        border-radius: 2px;
        padding: 3px;
    }
    QFileDialog QTreeView {
        background-color: #1A3652;
        color: white;
        border: 1px solid #27446d;
    }
    QFileDialog QTreeView::item:selected {
        background-color: #0078D4;
    }
    QFileDialog QPushButton {
        background-color: #1A3652;
        color: white;
        border: 1px solid #27446d;
        border-radius: 2px;
        padding: 5px 10px;
    }
    QFileDialog QPushButton:hover {
        background-color: #27446d;
    }
)";

} // namespace DesignToolbarStyles

#endif // DESIGNTOOLBAR_STYLES_H

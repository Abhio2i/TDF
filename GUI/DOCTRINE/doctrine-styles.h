/* ========================================================================= */
/* File: doctrine-styles.h                                                 */
/* Purpose: Stylesheet constants for the Doctrine Parameters panel           */
// Written by   : Arti Rajpoot
/* ========================================================================= */

#ifndef DOCTRINE_STYLES_H
#define DOCTRINE_STYLES_H

#include <QString>

namespace DoctrineStyles {

/* ============================================================================
   DOCTRINE PARAMETERS - DARK THEME
   Background : #0F2636
   Border     : #2A4A6B
   Text       : #D0E4F0  (inputs)  /  #A8C0D0 (labels)
   Accent     : #4A8AB0
   Blue Force : #1565C0  |  Red Force : #B71C1C
============================================================================ */

// %%% Main Panel Style %%%
const QString PanelStyle = R"(

    /* --- Root widget --- */
    DoctrineParameters {
        background-color: #0F2636;
        border: 9px solid white;
        border-radius: 4px;
    }

    /* --- Title bar label --- */
    QLabel#titleLabel {
        color: #C8D8E8;
        font-size: 13px;
        font-weight: bold;
        padding: 8px 10px;
        background-color: #1A3A52;
        border-bottom: 1px solid #2A4A6B;
        border-radius: 3px 3px 0 0;
    }

    /* --- Generic labels (field names) --- */
    QLabel {
        color: #A8C0D0;
        font-size: 12px;
        background-color: transparent;
    }

    /* --- Force selection bar background --- */
    QFrame#forceBar {
        background-color: #0D1F2E;
        border-bottom: 1px solid #2A4A6B;
    }

    /* --- Horizontal divider line --- */
    QFrame#divider {
        background-color: #2A4A6B;
        max-height: 1px;
    }

    /* ------------------------------------------------------------------ */
    /* TEXT INPUTS                                                          */
    /* ------------------------------------------------------------------ */
    QLineEdit {
        background-color: #162D40;
        color: #D0E4F0;
        border: 1px solid #2A4A6B;
        border-radius: 3px;
        padding: 4px 8px;
        font-size: 12px;
        selection-background-color: #3A6080;
    }
    QLineEdit:hover  { border: 1px solid #3A6A8A; }
    QLineEdit:focus  {
        border: 1px solid #4A8AB0;
        background-color: #1A3448;
    }

    /* ------------------------------------------------------------------ */
    /* COMBO BOXES                                                          */
    /* ------------------------------------------------------------------ */
    QComboBox {
        background-color: #162D40;
        color: #D0E4F0;
        border: 1px solid #2A4A6B;
        border-radius: 3px;
        padding: 4px 8px;
        font-size: 12px;
        min-width: 140px;
        min-height: 26px;
    }
    QComboBox:hover { border: 1px solid #3A6A8A; }
    QComboBox:focus { border: 1px solid #4A8AB0; }

    /* Drop-down button area */
    QComboBox::drop-down {
        subcontrol-origin: padding;
        subcontrol-position: right center;
        position: absolute;
        top: 0px; right: 0px; bottom: 0px;
        width: 24px;
        border-left: 1px solid #2A4A6B;
        background-color: #1A3A52;
        border-top-right-radius: 3px;
        border-bottom-right-radius: 3px;
    }
    QComboBox::drop-down:hover {
        background-color: #0078D4;
        border-left-color: #0078D4;
    }

    /* Arrow icon — uses project icon, falls back to CSS triangle */
    QComboBox::down-arrow {
        image: url(:/icons/images/down.png);
        width: 16px;
        height: 16px;
    }
    QComboBox::down-arrow:on {
        image: url(:/icons/images/up.png);
    }

    /* Popup list */
    QComboBox QAbstractItemView {
        background-color: #162D40;
        color: #D0E4F0;
        border: 1px solid #2A4A6B;
        border-radius: 3px;
        selection-background-color: #0078D4;
        selection-color: #FFFFFF;
        outline: none;
    }
    QComboBox QAbstractItemView::item {
        color: #D0E4F0;
        background-color: #162D40;
        padding: 6px 8px;
        border: none;
    }
    QComboBox QAbstractItemView::item:selected {
        background-color: #0078D4;
        color: #FFFFFF;
    }
    QComboBox QAbstractItemView::item:hover {
        background-color: #1E3E5E;
    }

    /* ------------------------------------------------------------------ */
    /* CLEAR ZONES BUTTON                                                   */
    /* ------------------------------------------------------------------ */
    QPushButton#clearZonesBtn {
        background-color: #1A3A52;
        color: #A8C0D0;
        border: 1px solid #2A4A6B;
        border-radius: 3px;
        padding: 5px 12px;
        font-size: 12px;
    }
    QPushButton#clearZonesBtn:hover {
        background-color: #243F58;
        border: 1px solid #3A6A8A;
        color: #C8D8E8;
    }
    QPushButton#clearZonesBtn:pressed {
        background-color: #0F2A3C;
    }

    /* ------------------------------------------------------------------ */
    /* SCROLLBAR                                                            */
    /* ------------------------------------------------------------------ */
    QScrollBar:vertical {
        background-color: #0F2636;
        width: 12px;
        border-radius: 6px;
    }
    QScrollBar::handle:vertical {
        background-color: #3A506B;
        min-height: 20px;
        border-radius: 6px;
    }
    QScrollBar::handle:vertical:hover { background-color: #4A607B; }

    QScrollBar:horizontal {
        background-color: #0F2636;
        height: 12px;
        border-radius: 6px;
    }
    QScrollBar::handle:horizontal {
        background-color: #3A506B;
        min-width: 20px;
        border-radius: 6px;
    }
    QScrollBar::handle:horizontal:hover { background-color: #4A607B; }

)";

/* ============================================================================
   FORCE TYPE RADIO BUTTONS  (applied individually via setStyleSheet)
============================================================================ */

// %%% Blue Force — Selected %%%
const QString BlueActive = R"(
    QRadioButton {
        color: #FFFFFF;
        font-size: 12px;
        font-weight: bold;
        background-color: #1565C0;
        border: 2px solid #42A5F5;
        border-radius: 4px;
        padding: 5px 18px 5px 10px;
        spacing: 8px;
    }
    QRadioButton::indicator {
        width: 14px;
        height: 14px;
        border-radius: 7px;
        border: 2px solid #90CAF9;
        background-color: #42A5F5;
    }
)";

// %%% Blue Force — Unselected %%%
const QString BlueInactive = R"(
    QRadioButton {
        color: #7090A8;
        font-size: 12px;
        font-weight: normal;
        background-color: #0D1F2E;
        border: 1px solid #1E3A50;
        border-radius: 4px;
        padding: 5px 18px 5px 10px;
        spacing: 8px;
    }
    QRadioButton::indicator {
        width: 14px;
        height: 14px;
        border-radius: 7px;
        border: 2px solid #2A4A6B;
        background-color: #0D1F2E;
    }
)";

// %%% Red Force — Selected %%%
const QString RedActive = R"(
    QRadioButton {
        color: #FFFFFF;
        font-size: 12px;
        font-weight: bold;
        background-color: #B71C1C;
        border: 2px solid #EF5350;
        border-radius: 4px;
        padding: 5px 18px 5px 10px;
        spacing: 8px;
    }
    QRadioButton::indicator {
        width: 14px;
        height: 14px;
        border-radius: 7px;
        border: 2px solid #EF9A9A;
        background-color: #EF5350;
    }
)";

// %%% Red Force — Unselected %%%
const QString RedInactive = R"(
    QRadioButton {
        color: #7090A8;
        font-size: 12px;
        font-weight: normal;
        background-color: #0D1F2E;
        border: 1px solid #1E3A50;
        border-radius: 4px;
        padding: 5px 18px 5px 10px;
        spacing: 8px;
    }
    QRadioButton::indicator {
        width: 14px;
        height: 14px;
        border-radius: 7px;
        border: 2px solid #2A4A6B;
        background-color: #0D1F2E;
    }
)";

} // namespace DoctrineStyles

#endif // DOCTRINE_STYLES_H

/* ========================================================================= */
/* File: tactical-styles.h                                                 */
/* Purpose: Stylesheet constants for the Tactical Rules panel                */
/* Written by: Arti Rajpoot                                                  */
/* ========================================================================= */

#ifndef TACTICAL_STYLES_H
#define TACTICAL_STYLES_H

#include <QString>

namespace TacticalStyles {

/* ============================================================================
   TACTICAL RULES - DARK THEME
   Background : #0F2636
   Border     : #2A4A6B
   Text       : #D0E4F0  (inputs)  /  #A8C0D0 (labels)
   Accent     : #0078D4
   Apply Btn  : #1565C0 (blue)
   Reset Btn  : #1A3A52 (dark)
   Unit label : #7A9AB0 (muted)
============================================================================ */

// %%% Main Panel Style %%%
const QString PanelStyle = R"(

    /* --- Root widget --- */
    TacticalRules {
        background-color: #0F2636;
        border: 1px solid #2A4A6B;
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

    /* --- Unit labels (km, %) --- */
    QLabel#unitLabel {
        color: #7A9AB0;
        font-size: 11px;
        background-color: transparent;
        padding-left: 3px;
    }

    /* --- Horizontal divider line --- */
    QFrame#divider {
        background-color: #2A4A6B;
        max-height: 1px;
    }

    /* ------------------------------------------------------------------ */
    /* SPINBOXES  (Max Range, Threshold %, Fuel %)                         */
    /* ------------------------------------------------------------------ */
    QDoubleSpinBox {
        background-color: #162D40;
        color: #D0E4F0;
        border: 1px solid #2A4A6B;
        border-radius: 3px;
        padding: 4px 6px;
        font-size: 12px;
        min-width: 70px;
        max-width: 100px;
        min-height: 26px;
    }
    QDoubleSpinBox:hover { border: 1px solid #3A6A8A; }
    QDoubleSpinBox:focus {
        border: 1px solid #4A8AB0;
        background-color: #1A3448;
    }

    /* Spinner up/down buttons */
    QDoubleSpinBox::up-button,
    QDoubleSpinBox::down-button {
        background-color: #1A3A52;
        border: none;
        width: 16px;
    }
    QDoubleSpinBox::up-button:hover,
    QDoubleSpinBox::down-button:hover {
        background-color: #0078D4;
    }

    /* Spinner arrows — use project icons if available */
    QDoubleSpinBox::up-arrow {
        image: url(:/icons/images/up.png);
        width: 10px;
        height: 10px;
    }
    QDoubleSpinBox::down-arrow {
        image: url(:/icons/images/down.png);
        width: 10px;
        height: 10px;
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
        min-width: 130px;
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
    /* APPLY CHANGES BUTTON  (blue primary)                                */
    /* ------------------------------------------------------------------ */
    QPushButton#applyBtn {
        background-color: #1565C0;
        color: #FFFFFF;
        border: 1px solid #42A5F5;
        border-radius: 3px;
        padding: 6px 16px;
        font-size: 12px;
        font-weight: bold;
        min-width: 110px;
        min-height: 28px;
    }
    QPushButton#applyBtn:hover {
        background-color: #1976D2;
        border: 1px solid #64B5F6;
    }
    QPushButton#applyBtn:pressed { background-color: #0D47A1; }

    /* ------------------------------------------------------------------ */
    /* RESET RULES BUTTON  (secondary dark)                                */
    /* ------------------------------------------------------------------ */
    QPushButton#resetBtn {
        background-color: #1A3A52;
        color: #A8C0D0;
        border: 1px solid #2A4A6B;
        border-radius: 3px;
        padding: 6px 16px;
        font-size: 12px;
        min-width: 90px;
        min-height: 28px;
    }
    QPushButton#resetBtn:hover {
        background-color: #243F58;
        border: 1px solid #3A6A8A;
        color: #C8D8E8;
    }
    QPushButton#resetBtn:pressed { background-color: #0F2A3C; }

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

} // namespace TacticalStyles

#endif // TACTICAL_STYLES_H

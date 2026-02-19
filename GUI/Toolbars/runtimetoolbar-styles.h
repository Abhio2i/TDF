/* ========================================================================= */
/* File: runtimetoolbar-styles.h                                            */
/* Purpose: Dark theme styles for RuntimeToolBar                            */
/* Background: #0F2636, Border: #27446d, Text: White, Accent: #0078D4       */
/* Icon size: 16x16 (smaller)                                               */
/* ========================================================================= */

#ifndef RUNTIMETOOLBAR_STYLES_H
#define RUNTIMETOOLBAR_STYLES_H

#include <QString>

namespace RuntimeToolbarStyles {

/* ============================================================================
   RUNTIME TOOLBAR DARK THEME
   Background: #0F2636, Border: #27446d, Text: White, Accent: #0078D4
============================================================================ */

/* Main Toolbar */
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

/* Toolbar Buttons */
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

/* Highlighted Button (active action) */
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

/* Time Label */
const QString TimeLabel = R"(
    QLabel {
        font-family: monospace;
        padding: 0 5px;
        color: white;
        background-color: transparent;
        font-size: 12px;
        font-weight: bold;
    }
    QLabel:hover {
        background-color: #1A3652;
        border-radius: 2px;
    }
)";

/* Simulation Status Label - RUNNING */
const QString StatusRunning = R"(
    QLabel {
        font-family: monospace;
        font-weight: bold;
        padding: 2px 8px;
        background-color: #1A4D1A;
        border: 1px solid #4CAF50;
        border-radius: 3px;
        color: #A5D6A5;
        font-size: 11px;
        min-width: 70px;
        text-align: center;
    }
)";

/* Simulation Status Label - RUNNING (blink variant) */
const QString StatusRunningBlink = R"(
    QLabel {
        font-family: monospace;
        font-weight: bold;
        padding: 2px 8px;
        background-color: #2D6A2D;
        border: 2px solid #4CAF50;
        border-radius: 3px;
        color: #C8E6C9;
        font-size: 11px;
        min-width: 70px;
        text-align: center;
    }
)";

/* Simulation Status Label - PAUSED */
const QString StatusPaused = R"(
    QLabel {
        font-family: monospace;
        font-weight: bold;
        padding: 2px 8px;
        background-color: #665C1A;
        border: 1px solid #FFC107;
        border-radius: 3px;
        color: #FFE082;
        font-size: 11px;
        min-width: 70px;
        text-align: center;
    }
)";

/* Simulation Status Label - PAUSED (blink variant) */
const QString StatusPausedBlink = R"(
    QLabel {
        font-family: monospace;
        font-weight: bold;
        padding: 2px 8px;
        background-color: #8B7E2D;
        border: 2px solid #FFC107;
        border-radius: 3px;
        color: #FFD54F;
        font-size: 11px;
        min-width: 70px;
        text-align: center;
    }
)";

/* Simulation Status Label - STOPPED */
const QString StatusStopped = R"(
    QLabel {
        font-family: monospace;
        font-weight: bold;
        padding: 2px 8px;
        background-color: #661A1A;
        border: 1px solid #F44336;
        border-radius: 3px;
        color: #FFABAB;
        font-size: 11px;
        min-width: 70px;
        text-align: center;
    }
)";

/* Speed Slider */
const QString SpeedSlider = R"(
    QSlider {
        background-color: transparent;
        min-width: 100px;
        max-width: 150px;
    }
    QSlider::groove:horizontal {
        border: 1px solid #27446d;
        height: 4px;
        background: #1A3652;
        margin: 2px 0;
        border-radius: 2px;
    }
    QSlider::handle:horizontal {
        background: #0078D4;
        border: 1px solid #3A5A7A;
        width: 12px;
        height: 12px;
        margin: -4px 0;
        border-radius: 6px;
    }
    QSlider::handle:horizontal:hover {
        background: #1A8AD9;
    }
    QSlider::sub-page:horizontal {
        background: #0078D4;
        border-radius: 2px;
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
        font-size: 12px;
        background-color: transparent;
    }
    QLineEdit {
        background-color: #1A3652;
        color: white;
        border: 1px solid #27446d;
        border-radius: 2px;
        padding: 5px;
        font-size: 12px;
        min-height: 20px;
    }
    QLineEdit:focus {
        border: 1px solid #0078D4;
        background-color: #1E3E5E;
    }
)";

/* Push Button */
const QString PushButton = R"(
    QPushButton {
        background-color: #1A3652;
        color: white;
        border: 1px solid #27446d;
        border-radius: 2px;
        padding: 5px 12px;
        font-size: 12px;
        min-width: 60px;
        min-height: 24px;
    }
    QPushButton:hover {
        background-color: #27446d;
        border-color: #3A5A7A;
    }
    QPushButton:pressed {
        background-color: #0078D4;
    }
    QPushButton:disabled {
        background-color: #333333;
        color: #666666;
        border: 1px solid #444444;
    }
)";

/* Speed Icon Button */
const QString SpeedIconButton = R"(
    QToolButton {
        border: none;
        padding: 0px;
        background-color: transparent;
    }
    QToolButton:hover {
        background-color: #1A3652;
        border-radius: 2px;
    }
)";

/* Tooltip Style */
const QString Tooltip = R"(
    QToolTip {
        background-color: #1A3652;
        color: white;
        border: 1px solid #27446d;
        border-radius: 2px;
        padding: 4px;
        font-size: 11px;
    }
)";

} // namespace RuntimeToolbarStyles

#endif // RUNTIMETOOLBAR_STYLES_H

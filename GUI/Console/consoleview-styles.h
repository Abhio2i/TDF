/* ========================================================================= */
/* File: consoleview-styles.h                                               */
/* Purpose: Dark theme styles for ConsoleView - COMPLETE TAB BAR FIX        */
/* Background: #0F2636, Border: #27446d, Text: White, Accent: #0078D4       */
// Written by   : Arti Rajpoot
/* ========================================================================= */

#ifndef CONSOLEVIEW_STYLES_H
#define CONSOLEVIEW_STYLES_H

#include <QString>

namespace ConsoleViewStyles {

/* ============================================================================
   CONSOLEVIEW DARK THEME - COMPLETE TAB BAR FIX
   Background: #0F2636, Border: #27446d, Text: White, Accent: #0078D4
============================================================================ */

/* Main Widget - Force dark everywhere */
const QString MainWidget = R"(
    QWidget {
        background-color: #0F2636;
        color: white;
    }
    QWidget * {
        background-color: #0F2636;
    }
)";

/* Tab Widget - Complete fix for entire tab area */
const QString TabWidget = R"(
    QTabWidget {
        background-color: #0F2636;
        border: none;
    }
    QTabWidget::pane {
        background-color: #0F2636;
        border: 1px solid #27446d;
        border-top: none;
        margin-top: -1px;
    }
    /* Force entire tab bar area including empty space */
    QTabBar {
        background-color: #0F2636 !important;
    }
    QTabBar::tab {
        background-color: #1A3652;
        color: white;
        border: 1px solid #27446d;
        border-bottom: none;
        border-top-left-radius: 3px;
        border-top-right-radius: 3px;
        padding: 6px 12px;
        margin-right: 2px;
        font-size: 12px;
        font-weight: normal;
    }
    QTabBar::tab:selected {
        background-color: #0F2636;
        color: white;
        border-bottom-color: #0F2636;
        font-weight: bold;
    }
    QTabBar::tab:hover {
        background-color: #27446d;
    }
    QTabBar::tab:!selected {
        margin-top: 2px;
    }
    /* Force ALL tab bar areas to dark */
    QTabBar::tear {
        background-color: #0F2636 !important;
    }
    QTabBar::scroller {
        background-color: #0F2636 !important;
    }
    QTabBar QToolButton {
        background-color: #0F2636;
        border: none;
    }
    /* The empty area to the right of tabs */
    QTabBar::tab:last {
        margin-right: 0px;
    }
    /* The entire tab bar frame */
    QTabBar::tab-bar {
        background-color: #0F2636;
    }
    /* Additional selectors for Qt versions */
    QTabBar::tab:first {
        margin-left: 0px;
    }
    /* The background behind all tabs */
    QTabBar::tab:selected {
        background-color: #0F2636;
    }
    /* Force the entire tab bar widget */
    QTabBar::tab-bar {
        background-color: #0F2636;
    }
QTabWidget::tab-bar {
    background-color: #0F2636;
    alignment: left;
}
/* Left corner area */
QTabWidget::left-corner {
    background-color: #0F2636;
}
/* Right corner area */
QTabWidget::right-corner {
    background-color: #0F2636;
}
)";

/* Text Edit (Console) */
const QString TextEdit = R"(
    QTextEdit {
        background-color: #0F2636;
        color: #E0E0E0;
        border: none;
        font-family: 'Courier New', monospace;
        font-size: 11px;
        selection-background-color: #0078D4;
        selection-color: white;
        padding: 4px;
    }
)";

/* Push Button - COMPACT VERSION */
const QString PushButton = R"(
    QPushButton {
        background-color: #1A3652;
        color: white;
        border: 1px solid #27446d;
        border-radius: 2px;
        padding: 3px 8px;
        font-size: 10px;
        min-width: 40px;
        min-height: 20px;
        max-height: 22px;
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

/* Button Layout Container */
const QString ButtonContainer = R"(
    QWidget {
        background-color: #0F2636;
    }
)";

/* Scrollbar */
const QString ScrollBar = R"(
    QScrollBar:vertical {
        background-color: #0F2636;
        width: 12px;
        border-radius: 6px;
    }
    QScrollBar::handle:vertical {
        background-color: #3A5A7A;
        min-height: 20px;
        border-radius: 6px;
    }
    QScrollBar::handle:vertical:hover {
        background-color: #4A6A8A;
    }
    QScrollBar:horizontal {
        background-color: #0F2636;
        height: 12px;
        border-radius: 6px;
    }
    QScrollBar::handle:horizontal {
        background-color: #3A5A7A;
        min-width: 20px;
        border-radius: 6px;
    }
    QScrollBar::handle:horizontal:hover {
        background-color: #4A6A8A;
    }
)";

} // namespace ConsoleViewStyles

#endif // CONSOLEVIEW_STYLES_H

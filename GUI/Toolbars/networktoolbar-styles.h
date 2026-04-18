/* ========================================================================= */
/* File: networktoolbar-styles.h                                            */
/* Purpose: Dark theme styles for NetworkToolbar                            */
/* Background: #0F2636, Border: #27446d, Text: White, Accent: #0078D4       */
/* Icon size: 16x16 (smaller)                                               */
/* Written by   : Arti Rajpoot                                               */
/* ========================================================================= */

#ifndef NETWORKTOOLBAR_STYLES_H
#define NETWORKTOOLBAR_STYLES_H

#include <QString>

namespace NetworkToolbarStyles {

/* ============================================================================
   NETWORK TOOLBAR DARK THEME
   Background: #0F2636, Border: #27446d, Text: White, Accent: #0078D4
============================================================================ */

/* Main Toolbar */
const QString Toolbar = R"(
    QToolBar {
        background-color: #0F2636;
        border: none;
        border-bottom: 1px solid #27446d;
        spacing: 5px;
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
        padding: 4px;
        min-width: 32px;
        min-height: 32px;
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
    QComboBox {
        background-color: #1A3652;
        color: white;
        border: 1px solid #27446d;
        border-radius: 2px;
        padding: 5px;
        font-size: 12px;
        min-height: 20px;
    }
    QComboBox:hover {
        background-color: #27446d;
        border-color: #3A5A7A;
    }
    QComboBox::drop-down {
        border: none;
        width: 20px;
        background-color: #1A3652;
        border-top-right-radius: 2px;
        border-bottom-right-radius: 2px;
    }
    QComboBox::down-arrow {
        width: 12px;
        height: 12px;
        border-left: 4px solid transparent;
        border-right: 4px solid transparent;
        border-top: 4px solid white;
    }
    QComboBox QAbstractItemView {
        background-color: #1A3652;
        color: white;
        border: 1px solid #27446d;
        selection-background-color: #0078D4;
        selection-color: white;
    }
    QComboBox QAbstractItemView::item {
        padding: 5px;
        color: white;
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

/* Table Widget (for Network Status) */
const QString TableWidget = R"(
    QTableWidget {
        background-color: #0F2636;
        color: white;
        border: 1px solid #27446d;
        gridline-color: #1A3652;
        font-size: 11px;
    }
    QTableWidget::item {
        padding: 4px;
        color: white;
        background-color: #0F2636;
        border-bottom: 1px solid #1A3652;
    }
    QTableWidget::item:selected {
        background-color: #0078D4;
        color: white;
    }
    QHeaderView::section {
        background-color: #1A3652;
        color: white;
        padding: 5px;
        border: none;
        border-bottom: 1px solid #27446d;
        font-weight: bold;
        font-size: 11px;
    }
)";

/* Message Box */
const QString MessageBox = R"(
    QMessageBox {
        background-color: #0F2636;
        color: white;
        border: 2px solid #27446d;
    }
    QMessageBox QLabel {
        color: white;
        font-size: 12px;
    }
    QPushButton {
        background-color: #1A3652;
        color: white;
        border: 1px solid #27446d;
        border-radius: 2px;
        padding: 5px 15px;
        font-size: 12px;
        min-width: 80px;
    }
    QPushButton:hover {
        background-color: #27446d;
    }
)";

/* Button Box */
const QString ButtonBox = R"(
    QDialogButtonBox {
        background-color: transparent;
    }
    QDialogButtonBox QPushButton {
        background-color: #1A3652;
        color: white;
        border: 1px solid #27446d;
        border-radius: 2px;
        padding: 5px 12px;
        font-size: 12px;
        min-width: 60px;
    }
    QDialogButtonBox QPushButton:hover {
        background-color: #27446d;
    }
)";

} // namespace NetworkToolbarStyles

#endif // NETWORKTOOLBAR_STYLES_H

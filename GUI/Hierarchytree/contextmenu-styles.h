/* ========================================================================= */
/* File: contextmenu-styles.h                                               */
/* Purpose: Dark theme styles for ContextMenu and related dialogs           */
/* Background: #0F2636, Border: #27446d, Text: White, Accent: #0078D4       */
/* ========================================================================= */

#ifndef CONTEXTMENU_STYLES_H
#define CONTEXTMENU_STYLES_H

#include <QString>

namespace ContextMenuStyles {

/* ============================================================================
   CONTEXT MENU DARK THEME
   Background: #0F2636, Border: #27446d, Text: White, Accent: #0078D4
============================================================================ */

/* Main Context Menu */
const QString ContextMenu = R"(
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
    }
    QMenu::separator {
        height: 1px;
        background-color: #27446d;
        margin: 4px 0;
    }
    QMenu::icon {
        margin-right: 8px;
    }
)";

/* QInputDialog (for Rename, Add Folder) */
const QString InputDialog = R"(
    QInputDialog {
        background-color: #0F2636;
        color: white;
        border: 2px solid #27446d;
        border-radius: 3px;
    }
    QInputDialog QLabel {
        color: white;
        font-size: 12px;
        background-color: transparent;
        padding: 5px;
    }
    QInputDialog QLineEdit {
        background-color: #1A3652;
        color: white;
        border: 1px solid #27446d;
        border-radius: 2px;
        padding: 6px;
        font-size: 12px;
        min-height: 20px;
        selection-background-color: #0078D4;
    }
    QInputDialog QLineEdit:focus {
        border: 1px solid #0078D4;
        background-color: #1E3E5E;
    }
    QInputDialog QPushButton {
        background-color: #1A3652;
        color: white;
        border: 1px solid #27446d;
        border-radius: 2px;
        padding: 6px 16px;
        font-size: 12px;
        min-width: 80px;
    }
    QInputDialog QPushButton:hover {
        background-color: #27446d;
        border-color: #3A5A7A;
    }
    QInputDialog QPushButton:pressed {
        background-color: #0078D4;
    }
    QInputDialog QPushButton:default {
        background-color: #0078D4;
        border: 1px solid #1A8AD9;
    }
    QInputDialog QComboBox {
        background-color: #1A3652;
        color: white;
        border: 1px solid #27446d;
        border-radius: 2px;
        padding: 5px;
        font-size: 12px;
        min-height: 20px;
    }
    QInputDialog QComboBox:hover {
        background-color: #27446d;
    }
    QInputDialog QComboBox::drop-down {
        border: none;
        width: 20px;
    }
    QInputDialog QComboBox::down-arrow {
        border-left: 4px solid transparent;
        border-right: 4px solid transparent;
        border-top: 4px solid white;
        width: 0;
        height: 0;
    }
    QInputDialog QComboBox QAbstractItemView {
        background-color: #1A3652;
        color: white;
        border: 1px solid #27446d;
        selection-background-color: #0078D4;
    }
)";

/* QMessageBox */
const QString MessageBox = R"(
    QMessageBox {
        background-color: #0F2636;
        color: white;
        border: 2px solid #27446d;
        border-radius: 3px;
    }
    QMessageBox QLabel {
        color: white;
        font-size: 12px;
        background-color: transparent;
        padding: 10px;
    }
    QMessageBox QPushButton {
        background-color: #1A3652;
        color: white;
        border: 1px solid #27446d;
        border-radius: 2px;
        padding: 6px 16px;
        font-size: 12px;
        min-width: 80px;
    }
    QMessageBox QPushButton:hover {
        background-color: #27446d;
        border-color: #3A5A7A;
    }
    QMessageBox QPushButton:pressed {
        background-color: #0078D4;
    }
    QMessageBox QPushButton:default {
        background-color: #0078D4;
        border: 1px solid #1A8AD9;
    }
    QMessageBox QIcon {
        margin: 10px;
    }
)";

/* QProgressDialog */
const QString ProgressDialog = R"(
    QProgressDialog {
        background-color: #0F2636;
        color: white;
        border: 2px solid #27446d;
        border-radius: 3px;
    }
    QProgressDialog QLabel {
        color: white;
        font-size: 12px;
        background-color: transparent;
        padding: 10px;
    }
    QProgressDialog QProgressBar {
        border: 1px solid #27446d;
        background-color: #1A3652;
        border-radius: 2px;
        text-align: center;
        color: white;
        font-size: 11px;
        min-height: 20px;
    }
    QProgressDialog QProgressBar::chunk {
        background-color: #0078D4;
        border-radius: 2px;
    }
    QProgressDialog QPushButton {
        background-color: #1A3652;
        color: white;
        border: 1px solid #27446d;
        border-radius: 2px;
        padding: 4px 12px;
        font-size: 11px;
        min-width: 60px;
    }
    QProgressDialog QPushButton:hover {
        background-color: #27446d;
    }
    QProgressDialog QPushButton:pressed {
        background-color: #0078D4;
    }
)";

/* QDialog (base style for AddItemDialog etc.) */
const QString Dialog = R"(
    QDialog {
        background-color: #0F2636;
        color: white;
        border: 2px solid #27446d;
        border-radius: 3px;
    }
    QDialog QLabel {
        color: white;
        font-size: 12px;
        background-color: transparent;
    }
    QDialog QLineEdit {
        background-color: #1A3652;
        color: white;
        border: 1px solid #27446d;
        border-radius: 2px;
        padding: 5px;
        font-size: 12px;
        min-height: 20px;
    }
    QDialog QLineEdit:focus {
        border: 1px solid #0078D4;
        background-color: #1E3E5E;
    }
    QDialog QComboBox {
        background-color: #1A3652;
        color: white;
        border: 1px solid #27446d;
        border-radius: 2px;
        padding: 5px;
        font-size: 12px;
        min-height: 20px;
    }
    QDialog QComboBox:hover {
        background-color: #27446d;
    }
    QDialog QComboBox::drop-down {
        border: none;
        width: 20px;
    }
    QDialog QComboBox::down-arrow {
        border-left: 4px solid transparent;
        border-right: 4px solid transparent;
        border-top: 4px solid white;
        width: 0;
        height: 0;
    }
    QDialog QComboBox QAbstractItemView {
        background-color: #1A3652;
        color: white;
        border: 1px solid #27446d;
        selection-background-color: #0078D4;
    }
    QDialog QCheckBox {
        color: white;
        spacing: 5px;
    }
    QDialog QCheckBox::indicator {
        width: 16px;
        height: 16px;
        background-color: #0A1A2A;
        border: 1px solid #4A6A8A;
        border-radius: 2px;
    }
    QDialog QCheckBox::indicator:checked {
        background-color: #0078D4;
        border: 1px solid #4DA6FF;
        image: url(data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAABAAAAAQCAYAAAAf8/9hAAAABHNCSVQICAgIfAhkiAAAAAlwSFlzAAAAdgAAAHYBTsfm/wAAABl0RVh0U29mdHdhcmUAd3d3Lmlua3NjYXBlLm9yZ5vuPBoAAADbSURBVDiNpZI9SgNBFMf/b3ZjI6wiiIiCVsFSWARB8AJ20iIWqXyA1IKVnSCEFFrYSQoR8gHCWlhY2imIhYVgIQgKViJBSRBS7I73d7PIgoT9mo93eG8e78GDMYb/kqTQnCQlkKQUFEWBpmk/kqQzYFs0JiDJCKk0jWIYxhgIABKSAEiaRlIURZZlVc/zsG3bJICiKIrrugiCwIiiiCRJMpkMptMpJEmCJEkQRRFRFMFxHEiSBNM0IYoiNputiKYIEXEcIwxD+L6PJEl4f39HlmVYr9fI8xy+78MwDEiSBK7rQlEUtNttlMtl9Ho9mKaJoihQr9fBcRwYY0iSBM1mE5ZlodPpYD6fY7FY4L9fBQApBmmhNlC8LAAAAABJRU5ErkJggg==);
    }
    QDialog QCheckBox::indicator:unchecked:hover {
        border: 1px solid #0078D4;
    }
    QDialog QPushButton {
        background-color: #1A3652;
        color: white;
        border: 1px solid #27446d;
        border-radius: 2px;
        padding: 6px 16px;
        font-size: 12px;
        min-width: 80px;
    }
    QDialog QPushButton:hover {
        background-color: #27446d;
        border-color: #3A5A7A;
    }
    QDialog QPushButton:pressed {
        background-color: #0078D4;
    }
    QDialog QPushButton:default {
        background-color: #0078D4;
        border: 1px solid #1A8AD9;
    }
    QDialog QDialogButtonBox {
        background-color: transparent;
    }
    QDialog QSpinBox, QDialog QDoubleSpinBox {
        background-color: #1A3652;
        color: white;
        border: 1px solid #27446d;
        border-radius: 2px;
        padding: 3px;
        min-height: 20px;
    }
    QDialog QSpinBox:focus, QDialog QDoubleSpinBox:focus {
        border: 1px solid #0078D4;
    }
)";

/* AddItemDialog specific styles (if needed) */
const QString AddItemDialog = R"(
    QDialog {
        background-color: #0F2636;
        color: white;
        border: 2px solid #27446d;
        border-radius: 3px;
        min-width: 400px;
    }
    QGroupBox {
        color: white;
        border: 1px solid #27446d;
        border-radius: 2px;
        margin-top: 10px;
        padding-top: 10px;
    }
    QGroupBox::title {
        subcontrol-origin: margin;
        left: 10px;
        padding: 0 5px;
        color: #CCCCCC;
    }
    QTabWidget::pane {
        border: 1px solid #27446d;
        background-color: #0F2636;
    }
    QTabBar::tab {
        background-color: #1A3652;
        color: white;
        border: 1px solid #27446d;
        border-bottom: none;
        padding: 8px 12px;
        border-top-left-radius: 2px;
        border-top-right-radius: 2px;
    }
    QTabBar::tab:selected {
        background-color: #0F2636;
        border-bottom-color: #0F2636;
    }
    QTabBar::tab:hover {
        background-color: #27446d;
    }
)";

} // namespace ContextMenuStyles

#endif // CONTEXTMENU_STYLES_H

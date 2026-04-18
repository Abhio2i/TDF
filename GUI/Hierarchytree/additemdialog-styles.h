/* ========================================================================= */
/* File: additemdialog-styles.h                                             */
/* Purpose: Dark theme styles for AddItemDialog - WITH CHECKMARK IMAGE      */
/* Background: #0F2636, Border: #27446d, Text: White, Accent: #0078D4       */
/* Written by   : Arti Rajpoot
/* ========================================================================= */

#ifndef ADDITEMDIALOG_STYLES_H
#define ADDITEMDIALOG_STYLES_H

#include <QString>

namespace AddItemDialogStyles {

/* ============================================================================
   ADDITEMDIALOG DARK THEME - WITH CHECKMARK IMAGE (LIKE DESIGNTOOLBAR)
   Background: #0F2636, Border: #27446d, Text: White, Accent: #0078D4
============================================================================ */

/* Main Dialog - Base Style */
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
        padding: 2px;
    }
    /* QLineEdit - FORCE WHITE TEXT IN ALL STATES */
    QDialog QLineEdit {
        background-color: #1A3652;
        color: white;
        border: 1px solid #27446d;
        border-radius: 2px;
        padding: 6px 8px;
        font-size: 12px;
        min-height: 20px;
        selection-background-color: #0078D4;
        selection-color: white;
    }
    QDialog QLineEdit:focus {
        border: 1px solid #0078D4;
        background-color: #1E3E5E;
        color: white;
    }
    QDialog QLineEdit:!focus {
        color: white;
    }
    QDialog QLineEdit:hover {
        background-color: #1E3E5E;
        color: white;
    }
    QDialog QLineEdit::placeholder {
        color: #B0B0B0;
        font-style: italic;
    }

    /* QComboBox - FORCE WHITE TEXT */
    QDialog QComboBox {
        background-color: #1A3652;
        color: white;
        border: 1px solid #27446d;
        border-radius: 2px;
        padding: 5px 8px;
        font-size: 12px;
        min-height: 20px;
        selection-background-color: #0078D4;
        selection-color: white;
    }
    QDialog QComboBox:hover {
        background-color: #27446d;
        border-color: #3A5A7A;
    }
    QDialog QComboBox:focus {
        border: 1px solid #0078D4;
        background-color: #1E3E5E;
        color: white;
    }
    QDialog QComboBox:!focus {
        color: white;
    }
QDialog QComboBox::down-arrow {
    image: url(:/icons/images/up.png);
    width: 12px;
    height: 12px;
    border: none;
}

QDialog QComboBox::down-arrow {
    image: url(:/icons/images/down.png);
    width: 12px;
    height: 12px;
    border: none;
}
    QDialog QComboBox QAbstractItemView {
        background-color: #1A3652;
        color: white;
        border: 1px solid #27446d;
        selection-background-color: #0078D4;
        selection-color: white;
        outline: none;
    }
    QDialog QComboBox QAbstractItemView::item {
        padding: 6px 10px;
        color: white;
    }
    QDialog QComboBox QAbstractItemView::item:selected {
        background-color: #0078D4;
        color: white;
    }
    QDialog QComboBox QAbstractItemView::item:hover {
        background-color: #27446d;
    }

    /* QCheckBox - UPDATED TO MATCH DESIGNTOOLBAR STYLE */
    QDialog QCheckBox {
        color: white;
        spacing: 8px;
        font-size: 12px;
    }
    QDialog QCheckBox::indicator {
        width: 16px;
        height: 16px;
        margin-right: 4px;
        subcontrol-position: left center;
        subcontrol-origin: padding;
        border: 2px solid #4A6A8A;
        border-radius: 4px;
        background-color: #0A1A2A;
    }
    QDialog QCheckBox::indicator:checked {
        background-color: #0078D4;
        border: 2px solid #4DA6FF;
        image: url(:/icons/images/check.png);  /* Same image as DesignToolbar */
    }
    QDialog QCheckBox::indicator:checked:hover {
        background-color: #1A8AD9;
        border: 2px solid #7AB8FF;
        image: url(:/icons/images/check.png);
    }
    QDialog QCheckBox::indicator:unchecked {
        background-color: #0A1A2A;
        border: 2px solid #4A6A8A;
        image: none;
    }
    QDialog QCheckBox::indicator:unchecked:hover {
        background-color: #1A3652;
        border: 2px solid #0078D4;
        image: none;
    }
    QDialog QCheckBox:disabled {
        color: #666666;
    }
    QDialog QCheckBox::indicator:disabled {
        background-color: #333333;
        border: 1px solid #555555;
        image: none;
    }
)";

const QString SimpleDialog = R"(
    QDialog {
        background-color: #0F2636;
        color: white;
        border: 2px solid #27446d;
        border-radius: 3px;
        min-width: 350px;
        min-height: 220px;
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
        padding: 6px 8px;
        font-size: 12px;
        min-height: 20px;
        selection-background-color: #0078D4;
        selection-color: white;
    }
    QLineEdit:focus {
        border: 1px solid #0078D4;
        background-color: #1E3E5E;
        color: white;
    }
    QLineEdit:!focus {
        color: white;
    }
    QComboBox {
        background-color: #1A3652;
        color: white;
        border: 1px solid #27446d;
        border-radius: 2px;
        padding: 5px 8px;
        font-size: 12px;
        min-height: 20px;
    }
    QComboBox:hover {
        background-color: #27446d;
    }
    QComboBox QAbstractItemView {
        background-color: #1A3652;
        color: white;
        border: 1px solid #27446d;
        selection-background-color: #0078D4;
        selection-color: white;
    }
    QComboBox QAbstractItemView::item {
        color: white;
    }
    /* QCheckBox for simple dialogs */
    QCheckBox {
        color: white;
        spacing: 8px;
        font-size: 12px;
    }
    QCheckBox::indicator {
        width: 16px;
        height: 16px;
        margin-right: 4px;
        subcontrol-position: left center;
        subcontrol-origin: padding;
        border: 2px solid #4A6A8A;
        border-radius: 4px;
        background-color: #0A1A2A;
    }
    QCheckBox::indicator:checked {
        background-color: #0078D4;
        border: 2px solid #4DA6FF;
        image: url(:/icons/images/check.png);
    }
    QCheckBox::indicator:checked:hover {
        background-color: #1A8AD9;
        border: 2px solid #7AB8FF;
        image: url(:/icons/images/check.png);
    }
    QCheckBox::indicator:unchecked {
        background-color: #0A1A2A;
        border: 2px solid #4A6A8A;
        image: none;
    }
    QCheckBox::indicator:unchecked:hover {
        background-color: #1A3652;
        border: 2px solid #0078D4;
        image: none;
    }
)";

/* Group Box */
const QString GroupBox = R"(
    QGroupBox {
        color: white;
        border: 1px solid #27446d;
        border-radius: 2px;
        margin-top: 12px;
        padding-top: 10px;
        font-weight: bold;
        font-size: 12px;
    }
    QGroupBox::title {
        subcontrol-origin: margin;
        left: 10px;
        padding: 0 5px;
        color: #CCCCCC;
        background-color: #0F2636;
    }
)";

/* Scroll Area */
const QString ScrollArea = R"(
    QScrollArea {
        background-color: #0F2636;
        border: none;
    }
    QScrollArea > QWidget > QWidget {
        background-color: #0F2636;
    }
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
        padding: 6px 16px;
        font-size: 12px;
        min-width: 80px;
        min-height: 24px;
    }
    QDialogButtonBox QPushButton:hover {
        background-color: #27446d;
        border-color: #3A5A7A;
    }
    QDialogButtonBox QPushButton:pressed {
        background-color: #0078D4;
    }
    QDialogButtonBox QPushButton:default {
        background-color: #0078D4;
        border: 1px solid #1A8AD9;
    }
    QDialogButtonBox QPushButton:disabled {
        background-color: #333333;
        color: #666666;
        border: 1px solid #444444;
    }
)";

/* Spin Box */
const QString SpinBox = R"(
    QSpinBox, QDoubleSpinBox {
        background-color: #1A3652;
        color: white;
        border: 1px solid #27446d;
        border-radius: 2px;
        padding: 3px 5px;
        font-size: 12px;
        min-height: 20px;
        selection-background-color: #0078D4;
        selection-color: white;
    }
    QSpinBox:focus, QDoubleSpinBox:focus {
        border: 1px solid #0078D4;
        background-color: #1E3E5E;
        color: white;
    }
    QSpinBox:!focus, QDoubleSpinBox:!focus {
        color: white;
    }
)";

/* Frame (for separators) */
const QString Frame = R"(
    QFrame {
        color: #27446d;
        background-color: #27446d;
        max-height: 1px;
    }
)";

/* Completer Popup */
const QString CompleterPopup = R"(
    QListView {
        background-color: #1A3652;
        color: white;
        border: 1px solid #27446d;
        outline: none;
    }
    QListView::item {
        padding: 6px 10px;
        color: white;
    }
    QListView::item:selected {
        background-color: #0078D4;
        color: white;
    }
    QListView::item:hover {
        background-color: #27446d;
    }
)";

/* Message Box */
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
    }
    QMessageBox QPushButton:pressed {
        background-color: #0078D4;
    }
)";

} // namespace AddItemDialogStyles

#endif // ADDITEMDIALOG_STYLES_H

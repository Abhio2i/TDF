/* ========================================================================= */
/* File: applicationdialog-styles.h                                         */
/* Purpose: Dark theme styles for ApplicationDialog                         */
/* Background: #0F2636, Border: #27446d, Text: White, Accent: #0078D4       */
/* ========================================================================= */

#ifndef APPLICATIONDIALOG_STYLES_H
#define APPLICATIONDIALOG_STYLES_H

#include <QString>

namespace ApplicationDialogStyles {

/* ============================================================================
   APPLICATION DIALOG DARK THEME
   Background: #0F2636, Border: #27446d, Text: White, Accent: #0078D4
============================================================================ */

/* Main Dialog */
const QString Dialog = R"(
    QDialog {
        background-color: #0F2636;
        color: white;
        border: 2px solid #27446d;
        border-radius: 3px;
        font-family: 'Segoe UI', Arial, sans-serif;
    }
)";

/* Group Box */
const QString GroupBox = R"(
    QGroupBox {
        color: white;
        font-weight: bold;
        border: 1px solid #27446d;
        border-radius: 4px;
        margin-top: 12px;
        padding-top: 10px;
        background-color: #0F2636;
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

/* Form Layout Label */
const QString FormLabel = R"(
    QLabel {
        color: white;
        font-size: 12px;
        background-color: transparent;
        padding: 2px;
    }
)";

/* Line Edit */
const QString LineEdit = R"(
    QLineEdit {
        background-color: #1A3652;
        color: white;
        border: 1px solid #27446d;
        border-radius: 3px;
        padding: 6px;
        font-size: 12px;
        min-width: 120px;
        selection-background-color: #0078D4;
        selection-color: white;
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

/* Line Edit - Error State */
const QString LineEditError = R"(
    QLineEdit {
        background-color: #1A3652;
        color: white;
        border: 1px solid #F44336;
        border-radius: 3px;
        padding: 6px;
        font-size: 12px;
        min-width: 120px;
    }
)";

/* Checkbox */
const QString CheckBox = R"(
    QCheckBox {
        color: white;
        spacing: 2px;
        font-size: 12px;
    }
    QCheckBox::indicator {
        width: 16px;
        height: 16px;
        margin-left: 1px;
        margin-right: 1px;
        subcontrol-position: left center;
        subcontrol-origin: padding;
        border: 2px solid #4A6A8A;
        border-radius: 4px;
        background-color: #0A1A2A;
    }
    QCheckBox::indicator:checked {
  image: url(:/icons/images/check.png);}

    QCheckBox::indicator:unchecked:hover {
           image: none;
    }
    QCheckBox::indicator:checked:hover {
       image: url(:/icons/images/check.png);
    }
)";

/* Error Label */
const QString ErrorLabel = R"(
    QLabel {
        color: #F44336;
        font-size: 11px;
        background-color: transparent;
        padding: 2px 0;
    }
)";

/* Separator Line */
const QString Separator = R"(
    QFrame {
        background-color: #27446d;
        max-height: 1px;
    }
)";

/* OK Button */
const QString OkButton = R"(
    QPushButton#okButton {
        background-color: #28a745;
        color: white;
        border: none;
        border-radius: 4px;
        padding: 8px 16px;
        min-width: 80px;
        font-size: 12px;
        font-weight: bold;
    }
    QPushButton#okButton:hover {
        background-color: #34ce57;
    }
    QPushButton#okButton:pressed {
        background-color: #1e7e34;
    }
    QPushButton#okButton:disabled {
        background-color: #4A4A4A;
        color: #888888;
        border: 1px solid #555555;
    }
)";

/* Cancel Button */
const QString CancelButton = R"(
    QPushButton#cancelButton {
        background-color: #1A3652;
        color: white;
        border: 1px solid #27446d;
        border-radius: 4px;
        padding: 8px 16px;
        min-width: 80px;
        font-size: 12px;
    }
    QPushButton#cancelButton:hover {
        background-color: #27446d;
        border-color: #3A5A7A;
    }
    QPushButton#cancelButton:pressed {
        background-color: #0078D4;
    }
)";

/* Button Layout */
const QString ButtonLayout = R"(
    QHBoxLayout {
        background-color: transparent;
    }
)";

/* Main Layout */
const QString MainLayout = R"(
    QVBoxLayout {
        background-color: transparent;
    }
)";

} // namespace ApplicationDialogStyles

#endif // APPLICATIONDIALOG_STYLES_H

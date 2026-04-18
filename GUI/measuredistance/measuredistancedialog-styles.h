/* ========================================================================= */
/* File: measuredistancedialog-styles.h                                     */
/* Purpose: Dark theme styles for MeasureDistanceDialog                     */
/* Background: #0F2636, Border: #27446d, Text: White, Accent: #0078D4       */
/* Written by   : Arti Rajpoot                                               */
/* ========================================================================= */

#ifndef MEASUREDISTANCEDIALOG_STYLES_H
#define MEASUREDISTANCEDIALOG_STYLES_H

#include <QString>

namespace MeasureDistanceDialogStyles {

/* ============================================================================
   MEASURE DISTANCE DIALOG DARK THEME
   Background: #0F2636, Border: #27446d, Text: White, Accent: #0078D4
============================================================================ */

/* Main Dialog */
const QString Dialog = R"(
    QDialog {
        background-color: #0F2636;
        color: white;
        border: 2px solid #27446d;
        border-radius: 3px;
    }
)";

/* Header Labels (x, y, Distance) */
const QString HeaderLabel = R"(
    QLabel {
        font-weight: bold;
        font-family: 'Courier New', monospace;
        color: white;
        background-color: transparent;
    }
)";

/* List Widget */
const QString ListWidget = R"(
    QListWidget {
        background-color: #1A3652;
        color: white;
        border: 1px solid #27446d;
        border-radius: 2px;
        font-family: 'Courier New', monospace;
        font-size: 11px;
        outline: none;
    }
    QListWidget::item {
        padding: 4px;
        color: white;
        background-color: #1A3652;
        border-bottom: 1px solid #27446d;
    }
    QListWidget::item:selected {
        background-color: #0078D4;
        color: white;
    }
    QListWidget::item:hover {
        background-color: #27446d;
    }
)";

/* Combo Box */
const QString ComboBox = R"(
    QComboBox {
        background-color: #1A3652;
        color: white;
        border: 1px solid #27446d;
        border-radius: 2px;
        padding: 5px;
        font-size: 11px;
        min-height: 20px;
    }
    QComboBox:hover {
        background-color: #27446d;
        border-color: #3A5A7A;
    }
    QComboBox:focus {
        border: 1px solid #0078D4;
    }
    QComboBox::drop-down {
        border: none;
        width: 20px;
        background-color: #1A3652;
        border-top-right-radius: 2px;
        border-bottom-right-radius: 2px;
    }
    QComboBox::down-arrow {
        border-left: 4px solid transparent;
        border-right: 4px solid transparent;
        border-top: 4px solid white;
        width: 0;
        height: 0;
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

/* Line Edit (Total Distance) */
const QString LineEdit = R"(
    QLineEdit {
        background-color: #1A3652;
        color: white;
        border: 1px solid #27446d;
        border-radius: 2px;
        padding: 5px;
        font-size: 11px;
        min-width: 100px;
    }
    QLineEdit:focus {
        border: 1px solid #0078D4;
    }
    QLineEdit:read-only {
        background-color: #0F2636;
        color: #E0E0E0;
    }
)";

/* Radio Button */
const QString RadioButton = R"(
    QRadioButton {
        color: white;
        spacing: 8px;
        font-size: 11px;
    }
    QRadioButton::indicator {
        width: 16px;
        height: 16px;
        background-color: #0A1A2A;
        border: 1px solid #4A6A8A;
        border-radius: 8px;
    }
    QRadioButton::indicator:checked {
        background-color: #0078D4;
        border: 1px solid #4DA6FF;
        image: url(data:image/svg+xml;utf8,<svg xmlns='http://www.w3.org/2000/svg' width='10' height='10' viewBox='0 0 10 10'><circle cx='5' cy='5' r='4' fill='white'/></svg>);
    }
    QRadioButton::indicator:unchecked:hover {
        border: 1px solid #0078D4;
    }
    QRadioButton::indicator:checked:hover {
        background-color: #1A8AD9;
    }
)";

/* Lock Button */
const QString LockButton = R"(
    QPushButton {
        border: none;
        background-color: transparent;
        padding: 2px;
    }
    QPushButton:hover {
        background-color: #1A3652;
        border-radius: 2px;
    }
    QPushButton:pressed {
        background-color: #27446d;
    }
)";

/* Push Buttons (Info, New, Copy, Close) */
const QString PushButton = R"(
    QPushButton {
        background-color: #1A3652;
        color: white;
        border: 1px solid #27446d;
        border-radius: 2px;
        padding: 6px 12px;
        font-size: 11px;
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

/* Total Label */
const QString TotalLabel = R"(
    QLabel {
        color: white;
        font-size: 11px;
        background-color: transparent;
    }
)";

} // namespace MeasureDistanceDialogStyles

#endif // MEASUREDISTANCEDIALOG_STYLES_H

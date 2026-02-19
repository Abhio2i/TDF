/* ========================================================================= */
/* File: entityinfodialog-styles.h                                          */
/* Purpose: Dark theme styles for EntityInfoDialog                          */
/* Background: #0F2636, Border: #27446d, Text: White, Accent: #0078D4       */
/* Checkmark image: :/icons/images/check.png                                */
/* ========================================================================= */

#ifndef ENTITYINFODIALOG_STYLES_H
#define ENTITYINFODIALOG_STYLES_H

#include <QString>

namespace EntityInfoDialogStyles {

/* ============================================================================
   ENTITY INFO DIALOG DARK THEME
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

/* Title Label */
const QString TitleLabel = R"(
    QLabel {
        font-weight: bold;
        font-size: 16px;
        color: white;
        padding: 10px;
        background-color: transparent;
    }
)";

/* Table Widget */
const QString TableWidget = R"(
    QTableWidget {
        background-color: #1A3652;
        color: white;
        border: 1px solid #27446d;
        gridline-color: #27446d;
    }
    QTableWidget::item {
        padding: 5px;
        color: white;
        background-color: #1A3652;
        border-bottom: 1px solid #27446d;
    }
    QTableWidget::item:selected {
        background-color: #0078D4;
        color: white;
    }
    QTableWidget::item:hover {
        background-color: #27446d;
    }
    QHeaderView::section {
        background-color: #0F2636;
        color: white;
        padding: 5px;
        border: none;
        border-bottom: 1px solid #27446d;
        font-weight: bold;
    }
)";

/* Label */
const QString Label = R"(
    QLabel {
        color: white;
        padding: 8px;
        background-color: transparent;
    }
)";

/* Position Label (specific) */
const QString PositionLabel = R"(
    QLabel {
        padding: 8px;
        color: #E0E0E0;
        background-color: #1A3652;
        border: 1px solid #27446d;
        border-radius: 2px;
        min-height: 35px;
    }
)";

/* Push Button */
const QString PushButton = R"(
    QPushButton {
        background-color: #1A3652;
        color: white;
        padding: 8px 12px;
        border: 1px solid #27446d;
        border-radius: 3px;
        margin: 2px;
        min-height: 35px;
    }
    QPushButton:hover {
        background-color: #27446d;
        border-color: #3A5A7A;
    }
    QPushButton:pressed {
        background-color: #0078D4;
    }
)";

/* Close Button (special) */
const QString CloseButton = R"(
    QPushButton {
        background-color: #1A3652;
        color: white;
        padding: 8px 16px;
        border: 1px solid #27446d;
        border-radius: 4px;
        font-weight: bold;
        min-width: 80px;
    }
    QPushButton:hover {
        background-color: #27446d;
        border-color: #3A5A7A;
    }
    QPushButton:pressed {
        background-color: #0078D4;
    }
)";

/* CheckBox */
const QString CheckBox = R"(
    QCheckBox {
        color: white;
        spacing: 8px;
    }
    QCheckBox::indicator {
        width: 16px;
        height: 16px;
        background-color: #0A1A2A;
        border: 1px solid #4A6A8A;
        border-radius: 2px;
    }
    QCheckBox::indicator:checked {
        background-color: #0078D4;
        border: 1px solid #4DA6FF;
        image: url(:/icons/images/check.png);
    }
    QCheckBox::indicator:unchecked:hover {
        border: 1px solid #0078D4;
    }
    QCheckBox::indicator:checked:hover {
        background-color: #1A8AD9;
        border: 1px solid #7AB8FF;
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
)";

/* Equipment Widget Container */
const QString EquipmentWidget = R"(
    QWidget {
        background-color: transparent;
    }
)";

/* Options Widget Container */
const QString OptionsWidget = R"(
    QWidget {
        background-color: transparent;
    }
)";

/* Sub-dialog Style (for Sensors, Radios, IFF, Formation dialogs) */
const QString SubDialog = R"(
    QDialog {
        background-color: #0F2636;
        color: white;
        border: 1px solid #27446d;
    }
    QLabel {
        color: white;
        font-weight: normal;
    }
    QLabel.title {
        font-size: 14px;
        color: white;
        font-weight: bold;
    }
    QLabel.summary {
        color: #B0B0B0;
        font-size: 11px;
    }
    QLabel.note {
        color: #F1C40F;
        background-color: #1A3652;
        padding: 8px;
        border: 1px solid #F1C40F;
        border-radius: 3px;
        font-size: 11px;
    }
)";

/* Sub-dialog Table */
const QString SubDialogTable = R"(
    QTableWidget {
        background-color: #1A3652;
        color: white;
        border: 1px solid #27446d;
        gridline-color: #27446d;
    }
    QTableWidget::item {
        padding: 4px;
        color: white;
        background-color: #1A3652;
        border-bottom: 1px solid #27446d;
    }
    QHeaderView::section {
        background-color: #0F2636;
        color: white;
        padding: 6px;
        border: none;
    }
)";

/* Sub-dialog Push Button */
const QString SubDialogButton = R"(
    QPushButton {
        background-color: #1A3652;
        color: white;
        padding: 6px 12px;
        border: 1px solid #27446d;
        border-radius: 3px;
    }
    QPushButton:hover {
        background-color: #27446d;
    }
    QPushButton:pressed {
        background-color: #0078D4;
    }
)";

/* No Data Label */
const QString NoDataLabel = R"(
    QLabel {
        color: #B0B0B0;
        padding: 20px;
        line-height: 1.5;
        border: 1px solid #27446d;
        background-color: #1A3652;
        border-radius: 5px;
    }
)";

} // namespace EntityInfoDialogStyles

#endif // ENTITYINFODIALOG_STYLES_H

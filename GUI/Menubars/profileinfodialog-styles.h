/* ========================================================================= */
/* File: profileinfodialog-styles.h                                         */
/* Purpose: Dark theme styles for ProfileInfoDialog                         */
/* Background: #0F2636, Border: #27446d, Text: White, Accent: #0078D4       */
/* ========================================================================= */

#ifndef PROFILEINFODIALOG_STYLES_H
#define PROFILEINFODIALOG_STYLES_H

#include <QString>

namespace ProfileInfoDialogStyles {

/* ============================================================================
   PROFILE INFO DIALOG DARK THEME
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

/* Title Label */
const QString TitleLabel = R"(
    QLabel {
        color: #CCCCCC;
        font-size: 16px;
        font-weight: bold;
        margin: 10px;
        background-color: transparent;
    }
)";

/* Text Edit (Performance Metrics) */
const QString TextEdit = R"(
    QTextEdit {
        background-color: #1A3652;
        color: #E0E0E0;
        border: 1px solid #27446d;
        border-radius: 3px;
        padding: 8px;
        font-family: 'Consolas', monospace;
        font-size: 11px;
        selection-background-color: #0078D4;
        selection-color: white;
    }
)";

/* Copy Button */
const QString CopyButton = R"(
    QPushButton {
        background-color: #1A3652;
        color: white;
        border: 1px solid #27446d;
        border-radius: 2px;
        padding: 6px 12px;
        font-size: 11px;
        font-weight: bold;
        min-width: 60px;
    }
    QPushButton:hover {
        background-color: #27446d;
        border-color: #3A5A7A;
    }
    QPushButton:pressed {
        background-color: #0078D4;
    }
)";

/* Close Button */
const QString CloseButton = R"(
    QPushButton {
        background-color: #8B3A3A;
        color: white;
        border: 1px solid #A54A4A;
        border-radius: 2px;
        padding: 6px 12px;
        font-size: 11px;
        font-weight: bold;
        min-width: 60px;
    }
    QPushButton:hover {
        background-color: #A54A4A;
        border-color: #B55A5A;
    }
    QPushButton:pressed {
        background-color: #C55A5A;
    }
)";

/* Status Label */
const QString StatusLabel = R"(
    QLabel {
        color: #B0B0B0;
        font-size: 10px;
        font-style: italic;
        padding: 5px;
        background-color: transparent;
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

} // namespace ProfileInfoDialogStyles

#endif // PROFILEINFODIALOG_STYLES_H

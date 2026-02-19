/* ========================================================================= */
/* File: projectinformation-styles.h                                        */
/* Purpose: Dark theme styles for Feedback/Project Information dialog       */
/* Background: #0F2636, Border: #27446d, Text: White, Accent: #0078D4       */
/* ========================================================================= */

#ifndef PROJECTINFORMATION_STYLES_H
#define PROJECTINFORMATION_STYLES_H

#include <QString>

namespace ProjectInformationStyles {

/* ============================================================================
   PROJECT INFORMATION DIALOG DARK THEME
   Background: #0F2636, Border: #27446d, Text: White, Accent: #0078D4
============================================================================ */

/* Main Dialog */
const QString Dialog = R"(
    QDialog {
        background-color: #0F2636;
        color: white;
        border: 2px solid #27446d;
        border-radius: 8px;
    }
)";

/* Title Label - "Name:" and "Version:" */
const QString TitleLabel = R"(
    QLabel {
        color: #CCCCCC;
        font-size: 11px;
        font-weight: bold;
        min-width: 80px;
        background-color: transparent;
    }
)";

/* Project Name Label */
const QString ProjectNameLabel = R"(
    QLabel {
        color: #7AB8FF;
        font-size: 11px;
        background-color: transparent;
    }
)";

/* Version Number Label */
const QString VersionLabel = R"(
    QLabel {
        color: #6FCF97;
        font-size: 11px;
        font-weight: bold;
        background-color: transparent;
    }
)";

/* Separator Line */
const QString Separator = R"(
    QFrame {
        background-color: #27446d;
        max-height: 1px;
    }
)";

/* OK Button - SMALLER SIZE */
const QString OkButton = R"(
    QPushButton {
        background-color: #1A3652;
        color: white;
        border: 1px solid #27446d;
        border-radius: 3px;
        padding: 5px 12px;
        font-size: 11px;
        font-weight: bold;
        min-width: 70px;  /* Reduced from 100px */
        min-height: 28px;  /* Reduced from 35px */
        max-width: 70px;
        max-height: 28px;
    }
    QPushButton:hover {
        background-color: #27446d;
        border-color: #3A5A7A;
    }
    QPushButton:pressed {
        background-color: #0078D4;
    }
)";

/* Main Layout */
const QString MainLayout = R"(
    QVBoxLayout {
        background-color: transparent;
    }
)";

/* Horizontal Layout */
const QString HBoxLayout = R"(
    QHBoxLayout {
        background-color: transparent;
    }
)";

} // namespace ProjectInformationStyles

#endif // PROJECTINFORMATION_STYLES_H

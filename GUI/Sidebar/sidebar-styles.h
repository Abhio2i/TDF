/* ========================================================================= */
/* File: sidebar-styles.h                                                   */
/* Purpose: Dark theme styles for SidebarWidget                             */
/* Background: #0F2636, Text: White, Accent: #0078D4                        */
/* Written by   : Arti Rajpoot                                               */
/* ========================================================================= */

#ifndef SIDEBAR_STYLES_H
#define SIDEBAR_STYLES_H

#include <QString>

namespace SidebarStyles {

/* Sidebar Widget Main Container */
const QString SidebarWidget = R"(
    QWidget {
        background-color: #0F2636;
    }
)";

/* Sidebar Button - Normal State */
const QString SidebarButton = R"(
    QPushButton {
        border: none;
        background-color: #0F2636;
        color: white;
        padding: 5px 10px;
        font-size: 12px;
        font-weight: 500;
    }
    QPushButton:hover {
        background-color: #1A3652;
        color: white;
    }
    QPushButton:checked {
        background-color: #27446d;
        color: white;
        border-left: 3px solid #0078D4;
        border-right: 3px solid #0078D4;
    }
    QPushButton:pressed {
        background-color: #1E3E5E;
        color: white;
    }
)";

/* Sidebar Button - Sensors Button (if special styling needed) */
const QString SensorsButton = R"(
    QPushButton {
        border: none;
        background-color: #0F2636;
        color: white;
        padding: 5px 10px;
        font-size: 12px;
        font-weight: 500;
    }
    QPushButton:hover {
        background-color: #1A3652;
        color: white;
    }
    QPushButton:checked {
        background-color: #27446d;
        color: white;
        border-left: 3px solid #0078D4;
        border-right: 3px solid #0078D4;
    }
    QPushButton:pressed {
        background-color: #1E3E5E;
        color: white;
    }
)";

/* Sidebar Button - Library Button */
const QString LibraryButton = R"(
    QPushButton {
        border: none;
        background-color: #0F2636;
        color: white;
        padding: 5px 10px;
        font-size: 12px;
        font-weight: 500;
    }
    QPushButton:hover {
        background-color: #1A3652;
        color: white;
    }
    QPushButton:checked {
        background-color: #27446d;
        color: white;
        border-left: 3px solid #0078D4;
        border-right: 3px solid #0078D4;
    }
    QPushButton:pressed {
        background-color: #1E3E5E;
        color: white;
    }
)";

/* Sidebar Button - Inspector Button */
const QString InspectorButton = R"(
    QPushButton {
        border: none;
        background-color: #0F2636;
        color: white;
        padding: 5px 10px;
        font-size: 12px;
        font-weight: 500;
    }
    QPushButton:hover {
        background-color: #1A3652;
        color: white;
    }
    QPushButton:checked {
        background-color: #27446d;
        color: white;
        border-left: 3px solid #0078D4;
        border-right: 3px solid #0078D4;
    }
    QPushButton:pressed {
        background-color: #1E3E5E;
        color: white;
    }
)";

/* Sidebar Button - TestScript Button */
const QString TestScriptButton = R"(
    QPushButton {
        border: none;
        background-color: #0F2636;
        color: white;
        padding: 5px 10px;
        font-size: 12px;
        font-weight: 500;
    }
    QPushButton:hover {
        background-color: #1A3652;
        color: white;
    }
    QPushButton:checked {
        background-color: #27446d;
        color: white;
        border-left: 3px solid #0078D4;
        border-right: 3px solid #0078D4;
    }
    QPushButton:pressed {
        background-color: #1E3E5E;
        color: white;
    }
)";

/* Sidebar Button Group */
const QString ButtonGroup = R"(
    QButtonGroup {
        background-color: transparent;
    }
)";

/* Sidebar Layout */
const QString SidebarLayout = R"(
    QHBoxLayout {
        margin: 0px;
        padding: 0px;
        spacing: 1px;
    }
)";

} // namespace SidebarStyles

#endif // SIDEBAR_STYLES_H

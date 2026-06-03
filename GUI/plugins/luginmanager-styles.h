/* ========================================================================= */
/* File:    pluginmanager-styles.h                                           */
/* Purpose: Dark theme styles for Plugin Manager dialog                      */
/* Background: #0F2636, Border: #27446d, Text: White, Accent: #00BFFF        */
/* Written by: Arti Rajpoot                                                  */
/* ========================================================================= */
#ifndef PLUGINMANAGER_STYLES_H
#define PLUGINMANAGER_STYLES_H

#include <QString>

namespace PluginManagerStyles {

/* ============================================================================
   PLUGIN MANAGER DIALOG DARK THEME
   Background: #0F2636, Border: #27446d, Text: White, Accent: #00BFFF
============================================================================ */

/* Main Dialog — transparent so custom glow border shows */
const QString Dialog = R"(
    QDialog {
        background-color: transparent;
        color: white;
    }
)";

/* Search Input */
const QString SearchInput = R"(
    QLineEdit {
        background-color: #1A3A4F;
        border: 2px solid #27446d;
        border-radius: 5px;
        color: white;
        padding: 6px 10px;
        font-size: 12px;
    }
    QLineEdit:focus {
        border-color: #00BFFF;
    }
)";

/* Plugin List Widget */
const QString PluginList = R"(
    QListWidget {
        background-color: #0A1E2E;
        border: none;
        color: #aac;
        font-size: 13px;
        outline: none;
    }
    QListWidget::item {
        padding: 9px 12px;
        border-left: 2px solid transparent;
        border-bottom: 0.5px solid #1A3A4F;
    }
    QListWidget::item:hover {
        background-color: #1A3A4F;
        color: white;
    }
    QListWidget::item:selected {
        background-color: #1A3A4F;
        color: #00BFFF;
        border-left: 2px solid #00BFFF;
    }
)";

/* Scroll Area */
const QString ScrollArea = R"(
    QScrollArea {
        background: transparent;
        border: none;
    }
)";

/* Section Label — "ALL PLUGINS", "TAGS" */
const QString SectionLabel = R"(
    QLabel {
        color: #6b93a8;
        font-size: 10px;
        letter-spacing: 1px;
        background-color: transparent;
    }
)";

/* Plugin Name in detail panel */
const QString PluginName = R"(
    QLabel {
        color: white;
        font-size: 17px;
        font-weight: bold;
        background-color: transparent;
    }
)";

/* Plugin Meta — version and author */
const QString PluginMeta = R"(
    QLabel {
        color: #8aabbb;
        font-size: 12px;
        background-color: transparent;
    }
)";

/* Plugin Description */
const QString PluginDesc = R"(
    QLabel {
        color: #aabbc8;
        font-size: 13px;
        background-color: transparent;
    }
)";

/* Status Badge */
const QString StatusBadge = R"(
    QLabel {
        border-radius: 10px;
        padding: 3px 12px;
        font-size: 11px;
        font-weight: bold;
    }
)";

/* Status Badge — Installed */
const QString StatusInstalled = R"(
    QLabel#statusBadge {
        background: #0d3d2a;
        color: #1D9E75;
        border-radius: 10px;
        padding: 3px 12px;
        font-size: 11px;
        font-weight: bold;
    }
)";

/* Status Badge — Update Available */
const QString StatusUpdate = R"(
    QLabel#statusBadge {
        background: #2a1e00;
        color: #d4a020;
        border-radius: 10px;
        padding: 3px 12px;
        font-size: 11px;
        font-weight: bold;
    }
)";

/* Status Badge — Not Installed */
const QString StatusNone = R"(
    QLabel#statusBadge {
        background: #1A3A4F;
        color: #6b93a8;
        border-radius: 10px;
        padding: 3px 12px;
        font-size: 11px;
        font-weight: bold;
    }
)";

/* Tag Pill */
const QString TagPill = R"(
    QLabel {
        background: #1A3A4F;
        color: #8aabbb;
        border-radius: 10px;
        padding: 3px 10px;
        font-size: 11px;
    }
)";

/* Topbar */
const QString Topbar = R"(
    QWidget#topbar {
        background-color: #071820;
        border-bottom: 1px solid #1A3A4F;
    }
)";

/* Title Label in Topbar */
const QString TopbarTitle = R"(
    QLabel {
        color: white;
        font-size: 14px;
        font-weight: bold;
        background-color: transparent;
    }
)";

/* Add Plugin Button */
const QString BtnAdd = R"(
    QPushButton {
        background-color: #00BFFF;
        color: #0F2636;
        border: none;
        border-radius: 6px;
        font-size: 13px;
        font-weight: 600;
        padding: 6px 14px;
    }
    QPushButton:hover  { background-color: #33ccff; }
    QPushButton:pressed { background-color: #0099cc; }
)";

/* Install Button */
const QString BtnInstall = R"(
    QPushButton {
        background-color: #00BFFF;
        color: #0F2636;
        border: none;
        border-radius: 6px;
        font-size: 13px;
        font-weight: 600;
        padding: 10px 20px;
    }
    QPushButton:hover  { background-color: #33ccff; }
    QPushButton:pressed { background-color: #0099cc; }
)";

/* Update Button */
const QString BtnUpdate = R"(
    QPushButton {
        background-color: transparent;
        color: #d4a020;
        border: 1px solid #d4a020;
        border-radius: 6px;
        font-size: 13px;
        font-weight: 500;
        padding: 10px 20px;
    }
    QPushButton:hover { background-color: #2a2000; }
)";

/* Uninstall Button */
const QString BtnUninstall = R"(
    QPushButton {
        background-color: transparent;
        color: #e06060;
        border: 1px solid #e06060;
        border-radius: 6px;
        font-size: 13px;
        font-weight: 500;
        padding: 10px 20px;
    }
    QPushButton:hover { background-color: #3a1a1a; }
)";

/* Remove Button */
const QString BtnRemove = R"(
    QPushButton {
        background-color: transparent;
        color: #666666;
        border: 1px solid #444444;
        border-radius: 6px;
        font-size: 12px;
        font-weight: 500;
        padding: 10px 20px;
    }
    QPushButton:hover   { background-color: #1a1a2a; color: #aaaaaa; border-color: #666666; }
    QPushButton:pressed { background-color: #2a0a0a; color: #e06060; border-color: #e06060; }
)";

/* Footer Widget */
const QString Footer = R"(
    QWidget#pluginFooter {
        background-color: #0F2636;
        border-top: 1px solid #1A3A4F;
    }
)";

/* Splitter Handle */
const QString Splitter = R"(
    QSplitter::handle {
        background: #1A3A4F;
        width: 1px;
    }
)";

/* Inner Container */
const QString Container = R"(
    QWidget {
        background-color: #0F2636;
    }
)";

/* Left Panel */
const QString LeftPanel = R"(
    QWidget {
        background-color: #0A1E2E;
    }
)";

/* Search Container */
const QString SearchContainer = R"(
    QWidget {
        background: #0A1E2E;
        border-bottom: 1px solid #1A3A4F;
    }
)";

/* Placeholder Icon */
const QString PlaceholderIcon = R"(
    QLabel {
        color: #3A5A7F;
        font-size: 36px;
        background-color: transparent;
    }
)";

/* Placeholder Text */
const QString PlaceholderText = R"(
    QLabel {
        color: #4A7A9F;
        font-size: 13px;
        background-color: transparent;
    }
)";

/* Separator Line */
const QString Separator = R"(
    QFrame {
        background-color: #1A3A4F;
        max-height: 1px;
    }
)";

} // namespace PluginManagerStyles

#endif // PLUGINMANAGER_STYLES_H

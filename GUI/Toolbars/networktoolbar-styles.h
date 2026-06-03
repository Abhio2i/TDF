/* =============================================================================
 * FILE:         distoolbar-styles.h
 * MODULE:       DIS Network Toolbar
 * PROJECT:      Tactical Display Framework (TDF)
 * ORGANISATION: Oxygen 2 Innovation (O2I)
 *
 * DESCRIPTION:  Dark theme styles for DISToolbar
 *               Matches existing NetworkToolbar theme exactly:
 *               Background: #0F2636, Border: #27446d, Text: White, Accent: #0078D4
 * =============================================================================
 */

#ifndef NETWORKTOOLBAR_STYLES_H
#define NETWORKTOOLBAR_STYLES_H

#include <QString>

namespace NetworkToolbarStyles {

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
        padding: 4px;
        font-size: 12px;
    }
    QLineEdit:focus {
        border: 1px solid #0078D4;
    }
    QSpinBox {
        background-color: #1A3652;
        color: white;
        border: 1px solid #27446d;
        border-radius: 2px;
        padding: 4px;
    }
    QSpinBox:focus {
        border: 1px solid #0078D4;
    }
    QComboBox {
        background-color: #1A3652;
        color: white;
        border: 1px solid #27446d;
        border-radius: 2px;
        padding: 4px;
    }
    QComboBox::drop-down {
        border: none;
    }
    QComboBox QAbstractItemView {
        background-color: #1A3652;
        color: white;
        selection-background-color: #0078D4;
    }
    QCheckBox {
        color: white;
        font-size: 12px;
    }
    QCheckBox::indicator {
        width: 14px;
        height: 14px;
        border: 1px solid #27446d;
        border-radius: 2px;
        background-color: #1A3652;
    }
    QCheckBox::indicator:checked {
        background-color: #0078D4;
        border: 1px solid #0078D4;
    }
    QGroupBox {
        color: white;
        border: 1px solid #27446d;
        border-radius: 3px;
        margin-top: 8px;
        padding-top: 8px;
        font-size: 12px;
        font-weight: bold;
    }
    QGroupBox::title {
        subcontrol-origin: margin;
        left: 8px;
        color: #0078D4;
    }
    QTabWidget::pane {
        border: 1px solid #27446d;
        background-color: #0F2636;
    }
    QTabBar::tab {
        background-color: #1A3652;
        color: white;
        padding: 6px 14px;
        border: 1px solid #27446d;
        border-bottom: none;
        font-size: 12px;
    }
    QTabBar::tab:selected {
        background-color: #0F2636;
        border-top: 2px solid #0078D4;
        color: #0078D4;
    }
    QTabBar::tab:hover {
        background-color: #27446d;
    }
    QTableWidget {
        background-color: #1A3652;
        color: white;
        border: 1px solid #27446d;
        gridline-color: #27446d;
    }
    QTableWidget::item:selected {
        background-color: #0078D4;
    }
    QHeaderView::section {
        background-color: #0F2636;
        color: #0078D4;
        border: 1px solid #27446d;
        padding: 4px;
        font-size: 11px;
        font-weight: bold;
    }
    QScrollBar:vertical {
        background-color: #0F2636;
        width: 8px;
        border: none;
    }
    QScrollBar::handle:vertical {
        background-color: #27446d;
        border-radius: 4px;
    }
)";

const QString ButtonBox = R"(
    QPushButton {
        background-color: #1A3652;
        color: white;
        border: 1px solid #27446d;
        border-radius: 3px;
        padding: 6px 16px;
        font-size: 12px;
        min-width: 80px;
    }
    QPushButton:hover {
        background-color: #27446d;
        border: 1px solid #0078D4;
    }
    QPushButton:pressed {
        background-color: #0078D4;
    }
    QPushButton:disabled {
        color: #666666;
        border-color: #444444;
    }
)";

const QString ConnectButton = R"(
    QPushButton {
        background-color: #0078D4;
        color: white;
        border: none;
        border-radius: 3px;
        padding: 6px 16px;
        font-size: 12px;
        font-weight: bold;
        min-width: 90px;
    }
    QPushButton:hover {
        background-color: #006CBE;
    }
    QPushButton:pressed {
        background-color: #005A9E;
    }
    QPushButton:disabled {
        background-color: #444444;
        color: #888888;
    }
)";

const QString DisconnectButton = R"(
    QPushButton {
        background-color: #C42B1C;
        color: white;
        border: none;
        border-radius: 3px;
        padding: 6px 16px;
        font-size: 12px;
        font-weight: bold;
        min-width: 90px;
    }
    QPushButton:hover {
        background-color: #A02318;
    }
    QPushButton:pressed {
        background-color: #8A1E15;
    }
    QPushButton:disabled {
        background-color: #444444;
        color: #888888;
    }
)";

const QString StatusConnected = R"(
    QLabel {
        color: #00C853;
        font-weight: bold;
        font-size: 12px;
    }
)";

const QString StatusDisconnected = R"(
    QLabel {
        color: #C42B1C;
        font-weight: bold;
        font-size: 12px;
    }
)";

} // namespace NetworkToolbarStyles

#endif // NETWORKTOOLBAR_STYLES_H

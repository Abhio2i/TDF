#ifndef LAYERPANEL_STYLES_H
#define LAYERPANEL_STYLES_H

#include <QString>

namespace LayerPanelStyles {

/* ============================================================================
   LAYER PANEL DARK THEME - WITH VISIBLE EXPAND/COLLAPSE ARROWS
   Background: #0F2636, Border: #27446d, Text: White, Accent: #0078D4
============================================================================ */

/* LayerPanel Main Dock Widget */
const QString LayerPanelDock = R"(
    QDockWidget {
        background-color: #0F2636;
        color: white;
        border: 2px solid #27446d;
    }
    QDockWidget::title {
        background-color: #0F2636;
        color: white;
        padding: 8px;
        border-bottom: 1px solid #1A3652;
        font-weight: bold;
        font-size: 13px;
        text-align: left;
    }
)";

/* Layer Tree Widget */
const QString LayerTree = R"(
    QTreeWidget {
        background-color: #0F2636;
        color: white;
        border: 2px solid #27446d;
        outline: none;
        show-decoration-selected: 1;
        font-size: 13px;
    }
    QTreeWidget::item {
        color: white;
        background-color: transparent;
        height: 26px;
        padding: 2px 2px;
        border: none;
        /* Leave right padding for the two icon buttons (▶ + ✓) */
        padding-right: 40px;
    }
    QTreeWidget::item:selected {
        background-color: #1C5FAE;
        color: white;
    }
    QTreeWidget::item:hover:!selected {
        background-color: #1A3652;
    }
    /* Hide Qt's built-in branch indicators — we use custom buttons */
    QTreeWidget::branch {
        background-color: #0F2636;
        image: none;
        border-image: none;
    }
    QTreeWidget::branch:selected {
        background-color: #1C5FAE;
    }
    /* Scrollbar */
    QScrollBar:vertical {
        background-color: #0F2636;
        width: 8px;
        border-radius: 4px;
        margin: 0;
    }
    QScrollBar::handle:vertical {
        background-color: #3A506B;
        min-height: 20px;
        border-radius: 4px;
    }
    QScrollBar::handle:vertical:hover { background-color: #4A607B; }
    QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical {
        border: none; background: none; height: 0px;
    }
    QScrollBar:horizontal {
        background-color: #0F2636;
        height: 8px;
        border-radius: 4px;
    }
    QScrollBar::handle:horizontal {
        background-color: #3A506B;
        min-width: 20px;
        border-radius: 4px;
    }
    QScrollBar::handle:horizontal:hover { background-color: #4A607B; }
    QScrollBar::add-line:horizontal, QScrollBar::sub-line:horizontal {
        border: none; background: none; width: 0px;
    }
)";

/* Root "Layers" Item - Non-selectable header */
const QString RootLayersItem = R"(
    QTreeWidgetItem {
        color: #CCCCCC;
        font-weight: bold;
        background-color: #1A3652;
        height: 28px;
    }
)";

/* Layer Item - Normal state */
const QString LayerItem = R"(
    QTreeWidgetItem {
        color: white;
        background-color: #0F2636;
        font-weight: normal;
        height: 26px;
    }
)";

/* Layer Item - Active state (selected layer) */
const QString LayerItemActive = R"(
    QTreeWidgetItem {
        color: white;
        background-color: #27446d;
        font-weight: bold;
        border-left: 3px solid #0078D4;
        height: 26px;
    }
)";

/* Shape Item - Base style for all shapes */
const QString ShapeItem = R"(
    QTreeWidgetItem {
        color: #E0E0E0;
        background-color: #0F2636;
        padding-left: 20px;
        font-size: 12px;
        height: 24px;
    }
)";

/* Shape Item - Circle (Orange) */
const QString ShapeItemCircle = R"(
    QTreeWidgetItem {
        color: #FFA500;
        background-color: #0F2636;
        padding-left: 20px;
        font-size: 12px;
        height: 24px;
    }
)";

/* Shape Item - Rectangle (Light Blue) */
const QString ShapeItemRectangle = R"(
    QTreeWidgetItem {
        color: #4DA6FF;
        background-color: #0F2636;
        padding-left: 20px;
        font-size: 12px;
        height: 24px;
    }
)";

/* Shape Item - Polygon (Green) */
const QString ShapeItemPolygon = R"(
    QTreeWidgetItem {
        color: #6FCF97;
        background-color: #0F2636;
        padding-left: 20px;
        font-size: 12px;
        height: 24px;
    }
)";

/* Shape Item - Line (Red) */
const QString ShapeItemLine = R"(
    QTreeWidgetItem {
        color: #FF6B6B;
        background-color: #0F2636;
        padding-left: 20px;
        font-size: 12px;
        height: 24px;
    }
)";

/* Shape Item - Point (Yellow) */
const QString ShapeItemPoint = R"(
    QTreeWidgetItem {
        color: #FFD966;
        background-color: #0F2636;
        padding-left: 20px;
        font-size: 12px;
        height: 24px;
    }
)";

/* Context Menu */
const QString ContextMenu = R"(
    QMenu {
        background-color: #0F2636;
        color: white;
        border: 1px solid #27446d;
        border-radius: 2px;
        font-size: 12px;
    }
    QMenu::item {
        padding: 6px 20px;
        color: white;
        background-color: transparent;
    }
    QMenu::item:selected {
        background-color: #0078D4;
        color: white;
    }
    QMenu::separator {
        height: 1px;
        background-color: #27446d;
        margin: 4px 0;
    }
)";

/* Visibility Toggle Button - Visible State (Green Check) */
const QString VisibilityToggleVisible = R"(
    QPushButton {
        border: none;
        background-color: transparent;
        color: #4CAF50;
        font-size: 14px;
        font-weight: bold;
        padding: 0px;
    }
    QPushButton:hover {
        background-color: #1A3652;
        border-radius: 2px;
    }
    QPushButton:pressed {
        color: #45a049;
    }
)";

/* Visibility Toggle Button - Hidden State (Red X) */
const QString VisibilityToggleHidden = R"(
    QPushButton {
        border: none;
        background-color: transparent;
        color: #F44336;
        font-size: 14px;
        font-weight: bold;
        padding: 0px;
    }
    QPushButton:hover {
        background-color: #1A3652;
        border-radius: 2px;
    }
    QPushButton:pressed {
        color: #d32f2f;
    }
)";

/* Input Dialog - Dark theme */
const QString InputDialog = R"(
    QInputDialog {
        background-color: #0F2636;
        color: white;
        border: 2px solid #27446d;
    }
    QLabel {
        color: white;
        font-size: 12px;
    }
    QLineEdit {
        background-color: #1A3652;
        color: white;
        border: 1px solid #27446d;
        border-radius: 2px;
        padding: 5px;
        font-size: 12px;
    }
    QLineEdit:focus {
        border: 1px solid #0078D4;
        background-color: #1E3E5E;
    }
    QPushButton {
        background-color: #1A3652;
        color: white;
        border: 1px solid #27446d;
        border-radius: 2px;
        padding: 5px 15px;
        font-size: 12px;
        min-width: 80px;
    }
    QPushButton:hover {
        background-color: #27446d;
    }
    QPushButton:pressed {
        background-color: #0078D4;
    }
)";

/* Message Box - Dark theme */
const QString MessageBox = R"(
    QMessageBox {
        background-color: #0F2636;
        color: white;
        border: 2px solid #27446d;
    }
    QMessageBox QLabel {
        color: white;
        font-size: 12px;
    }
    QPushButton {
        background-color: #1A3652;
        color: white;
        border: 1px solid #27446d;
        border-radius: 2px;
        padding: 5px 15px;
        font-size: 12px;
        min-width: 80px;
    }
    QPushButton:hover {
        background-color: #27446d;
    }
    QPushButton:pressed {
        background-color: #0078D4;
    }
)";

/* Progress Dialog - Dark theme */
const QString ProgressDialog = R"(
    QProgressDialog {
        background-color: #0F2636;
        color: white;
        border: 2px solid #27446d;
    }
    QLabel {
        color: white;
        font-size: 12px;
    }
    QProgressBar {
        border: 1px solid #27446d;
        background-color: #1A3652;
        text-align: center;
        color: white;
        font-size: 11px;
        border-radius: 2px;
    }
    QProgressBar::chunk {
        background-color: #0078D4;
        border-radius: 2px;
    }
)";

/* File Dialog - Dark theme (for export) */
const QString FileDialog = R"(
    QFileDialog {
        background-color: #0F2636;
        color: white;
    }
    QFileDialog QLabel {
        color: white;
    }
    QFileDialog QLineEdit {
        background-color: #1A3652;
        color: white;
        border: 1px solid #27446d;
        border-radius: 2px;
        padding: 3px;
    }
    QFileDialog QTreeView {
        background-color: #1A3652;
        color: white;
        border: 1px solid #27446d;
    }
    QFileDialog QTreeView::item:selected {
        background-color: #0078D4;
    }
    QFileDialog QPushButton {
        background-color: #1A3652;
        color: white;
        border: 1px solid #27446d;
        border-radius: 2px;
        padding: 5px 10px;
    }
    QFileDialog QPushButton:hover {
        background-color: #27446d;
    }
)";

/* ScrollBar - Dark theme */
const QString ScrollBar = R"(
    QScrollBar:vertical {
        background-color: #0F2636;
        width: 12px;
        border-radius: 6px;
    }
    QScrollBar::handle:vertical {
        background-color: #3A506B;
        min-height: 20px;
        border-radius: 6px;
    }
    QScrollBar::handle:vertical:hover {
        background-color: #4A607B;
    }
    QScrollBar::add-line:vertical,
    QScrollBar::sub-line:vertical {
        border: none;
        background: none;
    }
    QScrollBar:horizontal {
        background-color: #0F2636;
        height: 12px;
        border-radius: 6px;
    }
    QScrollBar::handle:horizontal {
        background-color: #3A506B;
        min-width: 20px;
        border-radius: 6px;
    }
    QScrollBar::handle:horizontal:hover {
        background-color: #4A607B;
    }
    QScrollBar::add-line:horizontal,
    QScrollBar::sub-line:horizontal {
        border: none;
        background: none;
    }
)";

} // namespace LayerPanelStyles

#endif // LAYERPANEL_STYLES_H

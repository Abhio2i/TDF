// inspector-styles.h
#ifndef INSPECTOR_STYLES_H
#define INSPECTOR_STYLES_H

#include <QString>

namespace InspectorStyles {

/* ============================================================================
   INSPECTOR DARK THEME - ORIGINAL COLORS
   Background: #0F2636, Border: #27446d, Text: White, Accent: #0078D4
============================================================================ */

/* Main Inspector Widget */
const QString InspectorWidget = R"(
    QDockWidget {
        background-color: transparent;
    }
    QDockWidget::title {
        background-color: #0F2636;
        color: white;
        padding: 0px;
        border: none;
        font-weight: bold;
        font-size: 13px;
        text-align: left;
    }
)";

/* Title Bar */
const QString TitleBar = R"(
    QWidget {
        background-color: #0F2636;
        border: 1px solid #27446d;
        border-radius: 2px;
    }
)";

/* Title Label */
const QString TitleLabel = R"(
    QLabel {
        font-size: 14px;
        font-weight: bold;
        color: white;
        background-color: #0F2636;
        padding: 6px 8px;
        letter-spacing: 0.3px;
    }
)";

/* Menu Button */
const QString MenuButton = R"(
    QPushButton {
        font-size: 18px;
        color: white;
        background-color: #0F2636;
        border: 1px solid #27446d;
        padding: 4px 10px;
    }
    QPushButton:hover {
        background-color: #1A3652;
        border-color: #3A5A7A;
    }
)";

/* Table Widget */
const QString TableWidget = R"(
    QTableWidget {
        background-color: #0F2636;
        color: white;
        border: none;
        gridline-color: #1A3652;
        outline: none;
        font-size: 12px;
    }
    QTableWidget::item {
        background-color: #0F2636;
        color: white;
        border-bottom: 1px solid #1A3652;
        padding: 6px 8px;
    }
    QTableWidget::item:selected {
        background-color: #0078D4;
        color: white;
    }
    QTableWidget::item:hover {
        background-color: #1A3652;
    }
    QHeaderView::section {
        background-color: #0F2636;
        color: #E0E0E0;
        padding: 8px;
        border: none;
        border-bottom: 2px solid #27446d;
        font-weight: 600;
        font-size: 12px;
    }
)";

/* Add Button */
const QString AddButton = R"(
    QPushButton {
        color: white;
        background-color: #1A3652;
        border: 1px solid #27446d;
        border-radius: 2px;
        padding: 2px 6px;
        font-size: 12px;
        font-weight: 500;
    }
    QPushButton:hover {
        background-color: #27446d;
        border-color: #3A5A7A;
    }
    QPushButton:pressed {
        background-color: #0F2636;
    }
)";

/* Remove Button */
const QString RemoveButton = R"(
    QPushButton {
        color: #FFB0B0;
        background-color: #1A3652;
        border: 1px solid #5A3A3A;
        border-radius: 2px;
        padding: 3px 8px;
        font-size: 11px;
    }
    QPushButton:hover {
        background-color: #4A2A2A;
        border-color: #8A4A4A;
        color: white;
    }
)";

/* ==================== COLOR TEMPLATE STYLES ==================== */
const QString ColorButton = R"(
    QPushButton {
        color: white;
        border: 1px solid #27446d;
        border-radius: 2px;
        padding: 4px 8px;
        font-size: 11px;
        font-weight: 500;
        min-width: 80px;
    }
    QPushButton:hover {
        border-color: #0078D4;
    }
)";

/* ==================== GEOCORDS TEMPLATE STYLES ==================== */
const QString GeocordsInput = R"(
    QLineEdit {
        background-color: #1A3652;
        color: white;
        border: 1px solid #27446d;
        border-radius: 2px;
        padding: 4px 8px;
        selection-background-color: #0078D4;
        font-size: 11px;
    }
    QLineEdit:focus {
        border: 1px solid #0078D4;
        background-color: #1E3E5E;
    }
)";

const QString GeocordsLabel = R"(
    QLabel {
        color: #B0B0B0;
        font-size: 11px;
        font-weight: 500;
        min-width: 35px;
    }
)";

/* ==================== ICONS DIALOG STYLES ==================== */
const QString IconsDialog_main = R"(
    QDialog {
        background-color: #0F2636;
        color: white;
        border: 2px solid #27446d;
        border-radius: 3px;
    }
    QToolTip {
        color: white;
        background-color: #1A3652;
        border: 1px solid #27446d;
        padding: 4px;
    }
)";

const QString IconsDialog_browseButton = R"(
    QPushButton {
        background-color: #1A3652;
        color: white;
        border: 1px solid #27446d;
        border-radius: 2px;
        padding: 8px 16px;
        font-weight: bold;
        font-size: 12px;
        min-width: 80px;
    }
    QPushButton:hover {
        background-color: #27446d;
        border-color: #3A5A7A;
    }
    QPushButton:pressed {
        background-color: #0F2636;
    }
)";

const QString IconsDialog_searchLabel = R"(
    QLabel {
        color: white;
        background: transparent;
        font-weight: bold;
        font-size: 12px;
    }
)";

const QString IconsDialog_searchBox = R"(
    QLineEdit {
        background-color: #1A3652;
        color: white;
        border: 1px solid #27446d;
        border-radius: 2px;
        padding: 8px;
        font-size: 12px;
    }
    QLineEdit:focus {
        border: 1px solid #0078D4;
    }
)";

const QString IconsDialog_listWidget = R"(
    QListWidget {
        background-color: #0F2636;
        color: white;
        border: 1px solid #27446d;
        border-radius: 3px;
        outline: none;
    }
    QListWidget::item {
        background-color: #1A3652;
        border: 1px solid #27446d;
        border-radius: 2px;
        margin: 2px;
        padding: 5px;
        color: white;
        text-align: center;
    }
    QListWidget::item:selected {
        background-color: #0078D4;
        border: 1px solid #0078D4;
    }
    QListWidget::item:hover {
        background-color: #27446d;
        border-color: #3A5A7A;
    }
)";

const QString IconsDialog_okButton = R"(
    QPushButton {
        background-color: #0078D4;
        color: white;
        border: none;
        border-radius: 2px;
        padding: 8px 16px;
        font-weight: bold;
        font-size: 12px;
        min-width: 80px;
    }
    QPushButton:hover {
        background-color: #1A8AD9;
    }
    QPushButton:pressed {
        background-color: #006ABC;
    }
)";

const QString IconsDialog_cancelButton = R"(
    QPushButton {
        background-color: #1A3652;
        color: white;
        border: 1px solid #27446d;
        border-radius: 2px;
        padding: 8px 16px;
        font-weight: bold;
        font-size: 12px;
        min-width: 80px;
    }
    QPushButton:hover {
        background-color: #27446d;
    }
    QPushButton:pressed {
        background-color: #0F2636;
    }
)";

const QString IconsDialog_noImagesLabel = R"(
    QLabel {
        color: white;
        background: transparent;
        font-size: 14px;
    }
)";

/* ==================== IMAGE TEMPLATE STYLES ==================== */
const QString ImagePreviewLabel = R"(
    QLabel {
        background-color: #1A3652;
        border: 1px solid #27446d;
        border-radius: 2px;
        color: white;
    }
)";

const QString ImageLineEdit = R"(
    QLineEdit {
        background-color: #1A3652;
        color: white;
        border: 1px solid #27446d;
        border-radius: 2px;
        padding: 4px 8px;
        selection-background-color: #0078D4;
        font-size: 11px;
    }
    QLineEdit:focus {
        border: 1px solid #0078D4;
        background-color: #1E3E5E;
    }
)";

const QString ImageBrowseButton = R"(
    QPushButton {
        background-color: #1A3652;
        color: white;
        border: 1px solid #27446d;
        border-radius: 2px;
        padding: 5px;
        font-size: 11px;
    }
    QPushButton:hover {
        background-color: #27446d;
        border-color: #3A5A7A;
    }
    QPushButton:pressed {
        background-color: #0F2636;
    }
)";

/* ==================== OPTION TEMPLATE STYLES ==================== */
const QString OptionComboBox = R"(
    QComboBox {
        background-color: #1A3652;
        color: white;
        border: 1px solid #27446d;
        border-radius: 2px;
        padding: 4px 8px;
        font-size: 11px;
        min-width: 120px;
    }
    QComboBox:hover {
        background-color: #27446d;
        border-color: #3A5A7A;
    }
    QComboBox::drop-down {
        border: none;
        width: 20px;
    }
    QComboBox::down-arrow {
        width: 12px;
        height: 12px;
    }
    QComboBox QAbstractItemView {
        background-color: #1A3652;
        color: white;
        border: 1px solid #27446d;
        selection-background-color: #0078D4;
        selection-color: white;
    }
    QComboBox QAbstractItemView::item {
        padding: 6px;
        color: white;
    }
    QComboBox QAbstractItemView::item:selected {
        background-color: #0078D4;
        color: white;
    }
)";

/* ==================== VECTOR TEMPLATE STYLES ==================== */
const QString VectorLabel = R"(
    QLabel {
        color: #B0B0B0;
        font-size: 11px;
        font-weight: 500;
        min-width: 20px;
    }
)";

const QString VectorInput = R"(
    QLineEdit {
        background-color: #1A3652;
        color: white;
        border: 1px solid #27446d;
        border-radius: 2px;
        padding: 4px 8px;
        selection-background-color: #0078D4;
        font-size: 11px;
        min-width: 100px;
        max-width: 150px;
    }
    QLineEdit:focus {
        border: 1px solid #0078D4;
        background-color: #1E3E5E;
    }
)";

/* ==================== INSPECTOR.CPP MISSING STYLES ==================== */

/* Generic Object Label - used in setupGenericObjectCell */
const QString GenericObjectLabel = R"(
    QLabel {
        color: #B0B0B0;
        font-size: 11px;
        min-width: 80px;
    }
)";

/* Unit Parameter Label - used in setupUnitParameterCell */
const QString UnitParamLabel = R"(
    QLabel {
        color: #999999;
        font-size: 11px;
        font-style: normal;
        padding-left: 4px;
    }
)";

/* Section Header - used in createSectionHeader */
const QString SectionHeader = R"(
    QWidget {
        background-color: #1A3652;
        border-left: 3px solid #0078D4;
        border-top: 1px solid #27446d;
        border-bottom: 1px solid #27446d;
    }
)";

/* Section Header Button - used in createSectionHeader */
const QString SectionHeaderButton = R"(
    QPushButton {
        background: transparent;
        border: none;
        color: white;
        font-size: 12px;
        font-weight: bold;
        padding: 2px 6px;
    }
    QPushButton:hover {
        color: white;
        background-color: #27446d;
        border-radius: 2px;
    }
)";

/* Section Header Label - used in createSectionHeader */
const QString SectionHeaderLabel = R"(
    QLabel {
        color: white;
        font-size: 13px;
        font-weight: 600;
        background: transparent;
        letter-spacing: 0.3px;
    }
)";

/* Container Dropdown Button - used in createSubcomponentWidget and handleMultiComponentContainer */
const QString ContainerDropdownButton = R"(
    QPushButton {
        text-align: left;
        padding: 8px 12px;
        background-color: #1A3652;
        border: 1px solid #27446d;
        border-radius: 2px;
        color: white;
        font-weight: 600;
        font-size: 12px;
    }
    QPushButton:hover {
        background-color: #27446d;
        border-color: #3A5A7A;
    }
    QPushButton:checked {
        background-color: #1E3E5E;
        border-left: 3px solid #0078D4;
    }
)";

/* Subcomponent Group Box - used in handleMultiComponentContainer */





/* ==================== CHECKBOX ==================== */
const QString CheckBox = R"(
    QCheckBox {
        color: white;
        border: none;
        background-color: transparent;
    }
    QCheckBox::indicator {
        width: 14px;
        height: 14px;
        border: 1px solid #666;
        background-color: white;
        subcontrol-origin: padding;
        subcontrol-position: center;
    }
    QCheckBox::indicator:checked {
        image: url(:/icons/images/check-box.png);
        background-color: #007bff;
        border: 1px solid #007bff;
    }
    QCheckBox::indicator:unchecked {
        image: none;
        background-color: white;
        border: 1px solid #666;
    }
    QCheckBox::indicator:hover {
        border: 1px solid #007bff;
    }
    QCheckBox::indicator:checked:hover {
        background-color: #1a8cff;
        border: 1px solid #1a8cff;
    }
)";

/* ==================== LINE EDIT ==================== */
const QString LineEdit = R"(
    QLineEdit {
        background-color: #1A3652;
        color: white;
        border: 1px solid #27446d;
        border-radius: 2px;
        padding: 4px 8px;
        selection-background-color: #0078D4;
        font-size: 12px;
    }
    QLineEdit:focus {
        border: 1px solid #0078D4;
        background-color: #1E3E5E;
    }
)";

/* Read-only Line Edit */
const QString ReadOnlyLineEdit = R"(
    QLineEdit {
        background-color: #0F2636;
        color: #C0C0C0;
        border: 1px solid #1A3652;
        border-radius: 2px;
        padding: 4px 8px;
        font-size: 12px;
    }
)";

/* Wheelable Line Edit */
const QString WheelableLineEdit = R"(
    QLineEdit {
        background-color: #1A3652;
        color: white;
        border: 1px solid #27446d;
        border-radius: 2px;
        padding: 4px 8px;
        selection-background-color: #0078D4;
        font-size: 12px;
    }
    QLineEdit:focus {
        border: 1px solid #0078D4;
        background-color: #1E3E5E;
    }
)";

/* Dropdown Button */
const QString DropdownButton = R"(
    QPushButton {
        color: white;
        background-color: #1A3652;
        border: 1px solid #27446d;
        border-radius: 2px;
        padding: 4px 8px;
        font-size: 11px;
        font-weight: 500;
        text-align: left;
    }
    QPushButton:hover {
        background-color: #27446d;
        border-color: #3A5A7A;
    }
    QPushButton:checked {
        background-color: #1E3E5E;
        border-left: 3px solid #0078D4;
    }

)";

/* List Widget */
const QString ListWidget = R"(
    QListWidget {
        background-color: #1A3652;
        color: white;
        border: 1px solid #27446d;
        border-radius: 2px;
        outline: none;
        font-size: 11px;
    }
    QListWidget::item {
        padding: 6px 8px;
        border-bottom: 1px solid #27446d;
        color: white;
    }
    QListWidget::item:selected {
        background-color: #0078D4;
        color: white;
    }
    QListWidget::item:hover {
        background-color: #27446d;
    }
)";

/* Dialog */
const QString Dialog = R"(
    QDialog {
        background-color: #0F2636;
        color: white;
        border: 2px solid #27446d;
        border-radius: 3px;
    }
    QLabel {
        color: white;
        font-weight: 500;
        font-size: 12px;
    }
    QDoubleSpinBox {
        background-color: #1A3652;
        color: white;
        border: 1px solid #27446d;
        border-radius: 2px;
        padding: 5px;
        font-size: 12px;
    }
    QDoubleSpinBox:focus {
        border: 1px solid #0078D4;
    }
    QPushButton {
        background-color: #1A3652;
        color: white;
        border: 1px solid #27446d;
        border-radius: 2px;
        padding: 6px 14px;
        font-size: 12px;
        font-weight: 500;
    }
    QPushButton:hover {
        background-color: #27446d;
    }
)";

/* Scroll Bar */
const QString ScrollBar = R"(
    QScrollBar:vertical {
        background-color: #0F2636;
        width: 12px;
        border: none;
    }
    QScrollBar::handle:vertical {
        background-color: #3A5A7A;
        min-height: 20px;
        border-radius: 6px;
    }
    QScrollBar::handle:vertical:hover {
        background-color: #4A6A8A;
    }
    QScrollBar:horizontal {
        background-color: #0F2636;
        height: 12px;
        border: none;
    }
    QScrollBar::handle:horizontal {
        background-color: #3A5A7A;
        min-width: 20px;
        border-radius: 6px;
    }
    QScrollBar::handle:horizontal:hover {
        background-color: #4A6A8A;
    }
)";

/* Table Key Item */
const QString TableKeyItem = R"(
    background-color: #1A3652;
    color: white;
    font-weight: 500;
    border-right: 1px solid #27446d;
)";

/* Table Parameter Key Item */
const QString TableParamKeyItem = R"(
    background-color: #0F2636;
    color: #E0E0E0;
    padding-left: 20px;
    border-right: 1px solid #27446d;
)";
/* ==================== FIXED DARK STYLES FOR SENSORS ==================== */

/* Subcomponent Group Box - Dark theme for sensors */
const QString SubcomponentGroupBox = R"(
    QGroupBox {
        background-color: #1A3652 !important;
        border: 1px solid #27446d !important;
        border-radius: 2px;
        margin-top: 12px;
        padding-top: 16px;
        font-weight: 600;
        color: white !important;
    }
    QGroupBox::title {
        subcontrol-origin: margin;
        left: 12px;
        padding: 0 8px;
        color: white !important;
        font-size: 12px;
        background-color: #1A3652 !important;
    }
    QGroupBox QLabel {
        color: white !important;
    }
    QGroupBox QWidget {
        background-color: #1A3652;
        color: white;
    }
)";

/* Default Group Box - Dark theme */
const QString DefaultGroupBox = R"(
    QGroupBox {
        background-color: #0F2636 !important;
        border: 1px solid #27446d !important;
        border-radius: 2px;
        margin-top: 8px;
        padding-top: 12px;
    }
    QGroupBox::title {
        subcontrol-origin: margin;
        left: 10px;
        padding: 0 6px;
        color: #E0E0E0 !important;
        font-size: 11px;
        font-weight: 500;
        background-color: #0F2636 !important;
    }
    QGroupBox QLabel {
        color: #E0E0E0 !important;
    }
)";

/* Empty Label - No sensors configured */
const QString EmptyLabel = R"(
    QLabel {
        color: #CCCCCC !important;
        background-color: #1A3652 !important;
        font-style: italic;
        padding: 20px;
        border: 1px solid #27446d;
        border-radius: 2px;
    }
)";

/* Container Widget - Dark background */
const QString ContainerWidget = R"(
    QWidget {
        background-color: #0F2636 !important;
        color: white !important;
    }
)";

/* Scroll Area - Dark background */
const QString ScrollArea = R"(
    QScrollArea {
        background-color: #0F2636 !important;
        border: 1px solid #27446d;
        border-radius: 2px;
    }
    QScrollArea QWidget {
        background-color: #0F2636 !important;
        color: white !important;
    }
)";

/* Sensor Active Checkbox */
const QString SensorCheckBox = R"(
    QCheckBox {
        color: white !important;
        spacing: 6px;
    }
    QCheckBox::indicator {
        width: 16px;
        height: 16px;
        background-color: #1A3652 !important;
        border: 1px solid #27446d !important;
        border-radius: 2px;
    }
    QCheckBox::indicator:checked {
        background-color: #0078D4 !important;
        border: 1px solid #0078D4 !important;
    }
    QCheckBox::indicator:unchecked:hover {
        border: 1px solid #0078D4 !important;
    }
)";

/* Sensor ID Label */
const QString SensorIdLabel = R"(
    QLabel {
        color: #B0B0B0 !important;
        font-size: 11px;
        min-width: 100px;
    }
)";

/* Sensor Value Label */
const QString SensorValueLabel = R"(
    QLabel {
        color: white !important;
        font-size: 11px;
        background-color: #1A3652 !important;
        padding: 3px 8px;
        border: 1px solid #27446d;
        border-radius: 2px;
    }
)";
} // namespace InspectorStyles

#endif // INSPECTOR_STYLES_H


#ifndef HIERARCHY_STYLES_H
#define HIERARCHY_STYLES_H

#include <QString>

namespace HierarchyStyles {

/* ============================================================================
   HIERARCHY TREE DARK THEME - SAME DIMENSIONS, ONLY COLORS CHANGED
   Background: #0F2636, Border: #27446d, Text: White, Accent: #0078D4
============================================================================ */

/* Search Bar - SAME padding, height, width, ONLY colors changed */
const QString SearchBar = R"(
    QLineEdit {
        background-color: #1A3652;
        color: white;
        border: 1px solid #27446d;
        border-radius: 3px;
        selection-background-color: #0078D4;
    }
    QLineEdit:focus {
        border: 1px solid #0078D4;
        background-color: #1E3E5E;
    }
    QLineEdit::placeholder {
        color: #B0B0B0;
        font-style: italic;
    }
    QLineEdit::clear-button {
        width: 16px;
        height: 16px;
    }
    QLineEdit::clear-button:hover {
        background-color: #27446d;
        border-radius: 2px;
    }
)";
const QString ProfileDropdown = R"(
    QComboBox {
        background-color: #1A3652;
        color: white;
        border: 1px solid #27446d;
        border-radius: 3px;
        min-height: 28px;

    }
    QComboBox:hover {
        background-color: #27446d;
        border-color: #3A5A7A;
    }
    QComboBox:focus {
        border: 1px solid #0078D4;
        background-color: #1E3E5E;
    }
    QComboBox::drop-down {
        subcontrol-origin: padding;
        subcontrol-position: right center;
        position: absolute;
        top: 0px;
        right: 0px;
        bottom: 0px;
        width: 24px;
        border-left: 1px solid #27446d;
        background-color: #1A3652;
        border-top-right-radius: 3px;
        border-bottom-right-radius: 3px;
    }
    QComboBox::drop-down:hover {
        background-color: #0078D4;
        border-left-color: #0078D4;
    }
    QComboBox::down-arrow {
        image: url(:/icons/images/down.png);  /* Your custom arrow icon */
        width: 16px;
        height: 16px;
    }
    QComboBox::down-arrow:on { /* When dropdown is open */
        image: url(:/icons/images/up.png);  /* Optional: up arrow when open */
    }
    /* Dropdown list */
    QComboBox QAbstractItemView {
        background-color: #1A3652;
        color: white;
        border: 1px solid #27446d;
        border-radius: 3px;
        selection-background-color: #0078D4;
        selection-color: white;
        outline: none;
    }
    QComboBox QAbstractItemView::item {
        color: white;
        background-color: #1A3652;
        padding: 8px;
        border: none;
    }
    QComboBox QAbstractItemView::item:selected {
        background-color: #0078D4;
        color: white;
    }
    QComboBox QAbstractItemView::item:hover {
        background-color: #27446d;
    }
)";
/* Tree Widget - Your existing styles */
const QString TreeWidget = R"(
    QTreeView {
        background-color: #0F2636;
        color: white;
        border: 2px solid #27446d;
        outline: none;
        show-decoration-selected: 1;
        alternate-background-color: #0F2636;
        font-size: 13px;
    }
    QTreeView::item {
        color: white;
        background-color: #0F2636;
        height: 26px;
        padding: 4px 2px;
        border: none;
    }
    QTreeView::item:selected {
        background-color: #0078D4;
        color: white;
        border-radius: 2px;
    }
    QTreeView::item:hover {
        background-color: #1A3652;
        border-radius: 2px;
    }
    QTreeView::branch:has-children:!has-siblings:closed,
    QTreeView::branch:closed:has-children:has-siblings {
        image: url(data:image/svg+xml;utf8,<svg width='12' height='12' viewBox='0 0 12 12' fill='white' xmlns='http://www.w3.org/2000/svg'><path d='M4 2L8 6L4 10Z'/></svg>);
    }
    QTreeView::branch:open:has-children:!has-siblings,
    QTreeView::branch:open:has-children:has-siblings {
        image: url(data:image/svg+xml;utf8,<svg width='12' height='12' viewBox='0 0 12 12' fill='white' xmlns='http://www.w3.org/2000/svg'><path d='M2 4L10 4L6 10Z'/></svg>);
    }
    QTreeView:focus {
        border: none;
        outline: none;
    }
)";

/* Tree Header */
const QString TreeHeader = R"(
    QHeaderView::section {
        background-color: #0F2636;
        color: white;
        padding: 8px;
        border: #0F2636;
        border-bottom: 1px solid #1A3652;
        font-weight: bold;
        font-size: 14px;
    }
)";

/* Filter Layout - Dark background */
const QString FilterLayout = R"(
    QWidget {
        background-color: #0F2636;
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
)";

} // namespace HierarchyStyles

#endif // HIERARCHY_STYLES_H

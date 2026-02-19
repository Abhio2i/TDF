// database-styles.h
#ifndef DATABASE_STYLES_H
#define DATABASE_STYLES_H
#include <QString>
namespace DatabaseStyles {
/* Database Editor Minimal CSS - Only for fixing visible issues */
/* Background: #0F2636, Border: #27446d */
const QString DatabaseEditorWidget = R"(
    /* Main Window Background */
    QMainWindow {
        background-color: #0F2636;
    }
    /* Menu Bar */
    QMenuBar {
        background-color: #0F2636;
        color: white;
        border-bottom: 1px solid #27446d;
    }
    QMenuBar::item {
        background-color: transparent;
        color: white;
        padding: 6px 12px;
    }
    QMenuBar::item:selected {
        background-color: #1A3652;
    }
    /* Dock Widget Title Bar */
    QDockWidget {
        color: white;
    }
    QDockWidget::title {
        background-color: #0F2636;
        color: white;
        padding: 6px;
        border-bottom: 1px solid #27446d;
        text-align: left;
    }
    /* Status Bar */
    QStatusBar {
        background-color: #0F2636;
        color: white;
        border-top: 1px solid #27446d;
    }
    /* Scroll Area - removes white background */
    QScrollArea {
        background-color: #0F2636;
        border: none;
    }
    /* Frame - removes white background */
    QFrame {
        background-color: transparent;
    }
    /* Dock Panel Context Menu (right-click on dock/toolbar) */
    QMenu {
        background-color: #1A3652;
        color: white;
        border: 1px solid #cccccc;
        border-radius: 3px;
        padding: 5px 0px;
        font-size: 12px;
    }
    QMenu::item {
        background-color: transparent;
        color: white;
        padding: 6px 30px 6px 30px;
        margin: 2px 5px;
        border-radius: 2px;
    }
    QMenu::item:selected {
        background-color: #27446d;
        color: white;
    }
    QMenu::item:checked {
        background-color: #1A3652;
        color: white;
        font-weight: bold;
    }
    QMenu::item:checked:selected {
        background-color: #27446d;
    }
    QMenu::indicator {
        width: 16px;
        height: 16px;
        margin-left: 5px;
    }
    QMenu::indicator:unchecked {
        image: none;
        border: 1px solid #999999;
        background-color: white;
        border-radius: 2px;
    }
    QMenu::indicator:checked {
        image: url(:/icons/images/check.png);
        border: 1px solid #0078D4;
        background-color: #0078D4;
        border-radius: 2px;
    }
    QMenu::separator {
        height: 1px;
        background-color: #e0e0e0;
        margin: 5px 10px;
    }
)";

const QString ContextMenu = R"(
    QMenu {
        background-color: #1A3652;
        color: white;
        border: 1px solid #cccccc;
        border-radius: 3px;
        padding: 5px 0px;
        font-size: 12px;
    }
    QMenu::item {
        background-color: transparent;
        color: white;
        padding: 6px 30px 6px 30px;
        margin: 2px 5px;
        border-radius: 2px;
    }
    QMenu::item:selected {
        background-color: #27446d;
        color: white;
    }
    QMenu::item:checked {
        background-color: #1A3652;
        color: white;
        font-weight: bold;
    }
    QMenu::item:checked:selected {
        background-color: #27446d;
    }
    QMenu::indicator {
        width: 16px;
        height: 16px;
        margin-left: 5px;
    }
    QMenu::indicator:unchecked {
        image: none;
        border: 1px solid #999999;
        background-color: white;
        border-radius: 2px;
    }
    QMenu::indicator:checked {
        image: url(:/icons/images/check.png);
        border: 1px solid #0078D4;
        background-color: #0078D4;
        border-radius: 2px;
    }
    QMenu::separator {
        height: 1px;
        background-color: #e0e0e0;
        margin: 5px 10px;
    }
)";
} // namespace DatabaseStyles
#endif // DATABASE_STYLES_H

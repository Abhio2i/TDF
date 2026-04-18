/* ========================================================================= */
/* File: mainwindow-styles.h                                                */
/* Purpose: Dark theme styles for MainWindow and its components             */
/* Background: #0F2636, Border: #27446d, Text: White, Accent: #0078D4       */
/* Written by   : Arti Rajpoot                                               */
/* ========================================================================= */

#ifndef MAINWINDOW_STYLES_H
#define MAINWINDOW_STYLES_H

#include <QString>

namespace MainWindowStyles {

/* ============================================================================
   MAINWINDOW DARK THEME
   Background: #0F2636, Border: #27446d, Text: White, Accent: #0078D4
============================================================================ */

/* Main Window */
const QString MainWindow = R"(
    QMainWindow {
        background-color: #0F2636;
        color: white;
    }
)";

/* Top Bar Widget */
const QString TopBarWidget = R"(
    QWidget#topBarWidget {
        background-color: #0F2636;
        border-bottom: 1px solid #27446d;
    }
)";

const QString MenuBar = R"(
    QMenuBar {
        background-color: transparent;
        color: white;
        padding: 5px 10px;
    }
    QMenuBar::item {
        padding: 8px 12px;
        background-color: transparent;
        color: white;
    }
    QMenuBar::item:selected {
        background-color: #1A3652;
        border-radius: 4px;
    }
    QMenuBar::item:pressed {
        background-color: #27446d;
    }
)";

/* Stacked Widget */
const QString StackedWidget = R"(
    QStackedWidget {
        background-color: #0F2636;
        border: none;
    }
)";

const QString NavigationPage = R"(
    QWidget {
        background-color: transparent;
    }
)";

/* Central Widget */
const QString CentralWidget = R"(
    QWidget {
        background-color: #0F2636;
    }
)";

const QString MessageBox = R"(
    QMessageBox {
        background-color: #0F2636;
        color: white;
        border: 2px solid #27446d;
        border-radius: 3px;
    }
    QMessageBox QLabel {
        color: white;
        font-size: 12px;
        background-color: transparent;
        padding: 10px;
    }
    QMessageBox QPushButton {
        background-color: #1A3652;
        color: white;
        border: 1px solid #27446d;
        border-radius: 2px;
        padding: 6px 16px;
        font-size: 12px;
        min-width: 80px;
    }
    QMessageBox QPushButton:hover {
        background-color: #27446d;
        border-color: #3A5A7A;
    }
    QMessageBox QPushButton:pressed {
        background-color: #0078D4;
    }
    QMessageBox QPushButton:default {
        background-color: #0078D4;
        border: 1px solid #1A8AD9;
    }
)";

/* QFileDialog */
const QString FileDialog = R"(
    QFileDialog {
        background-color: #0F2636;
        color: white;
        border: 2px solid #27446d;
    }
    QFileDialog QLabel {
        color: white;
        font-size: 12px;
    }
    QFileDialog QLineEdit {
        background-color: #1A3652;
        color: white;
        border: 1px solid #27446d;
        border-radius: 2px;
        padding: 5px;
    }
    QFileDialog QLineEdit:focus {
        border: 1px solid #0078D4;
    }
    QFileDialog QTreeView {
        background-color: #1A3652;
        color: white;
        border: 1px solid #27446d;
    }
    QFileDialog QTreeView::item:selected {
        background-color: #0078D4;
        color: white;
    }
    QFileDialog QTreeView::item:hover {
        background-color: #27446d;
    }
    QFileDialog QPushButton {
        background-color: #1A3652;
        color: white;
        border: 1px solid #27446d;
        border-radius: 2px;
        padding: 5px 12px;
        font-size: 12px;
        min-width: 80px;
    }
    QFileDialog QPushButton:hover {
        background-color: #27446d;
    }
    QFileDialog QPushButton:pressed {
        background-color: #0078D4;
    }
)";

/* QProgressDialog (for long operations) */
const QString ProgressDialog = R"(
    QProgressDialog {
        background-color: #0F2636;
        color: white;
        border: 2px solid #27446d;
        border-radius: 3px;
    }
    QProgressDialog QLabel {
        color: white;
        font-size: 12px;
        background-color: transparent;
        padding: 10px;
    }
    QProgressDialog QProgressBar {
        border: 1px solid #27446d;
        background-color: #1A3652;
        border-radius: 2px;
        text-align: center;
        color: white;
        font-size: 11px;
        min-height: 20px;
    }
    QProgressDialog QProgressBar::chunk {
        background-color: #0078D4;
        border-radius: 2px;
    }
    QProgressDialog QPushButton {
        background-color: #1A3652;
        color: white;
        border: 1px solid #27446d;
        border-radius: 2px;
        padding: 4px 10px;
        font-size: 11px;
    }
    QProgressDialog QPushButton:hover {
        background-color: #27446d;
    }
)";
const QString ToolTip = R"(
    QToolTip {
        background-color: #1A3652;
        color: white;
        border: 1px solid #0078D4;
        border-radius: 2px;
        padding: 2px 4px;
        font-size: 11px;
    }
)";

} // namespace MainWindowStyles

#endif // MAINWINDOW_STYLES_H

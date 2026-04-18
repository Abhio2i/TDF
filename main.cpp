/* ========================================================================= */
/* File: main.cpp                                                            */
/* Purpose: Application entry point - initializes QApplication, applies dark */
/*          theme, installs message handler, and shows main window           */
/* Written by   : Arti Rajpoot                                               */
/* ========================================================================= */
#include "GUI/mainwindow.h"
#include <QApplication>
#include "core/Debug/console.h"
#include <string>
#include "Setup.h"
void customMessageHandler(QtMsgType type, const QMessageLogContext &context, const QString &msg);
int main(int argc, char *argv[])
{
    qRegisterMetaType<std::string>("std::string");
    QApplication a(argc, argv);

    // --- INLINE DARK THEME CSS ---
    QString globalDarkStyle = R"(
        /* Poori App ka background */
        QWidget {
            background-color: #0F2636;
            color: #E0E0E0;
            font-family: 'Segoe UI', sans-serif;
        }

        /* Buttons styling */
        QPushButton {
            background-color: #1A3652;
            border: 1px solid #27446d;
            border-radius: 4px;
            padding: 5px 15px;
            color: white;
        }

        QPushButton:hover {
            background-color: #27446d;
        }
        QPushButton:pressed {
            background-color: #0078D4;
        }

        /* Input fields (LineEdit, TextEdit) */
        QLineEdit, QTextEdit, QPlainTextEdit {
            background-color: #0A1A26;
            border: 1px solid #27446d;
            color: white;
            padding: 4px;
        }

        /* Scrollbars */
        QScrollBar:vertical {
            border: none;
            background: #0F2636;
            width: 10px;
        }
        QScrollBar::handle:vertical {
            background: #27446d;
            min-height: 20px;
            border-radius: 5px;
        }

        /* Menu Bar */
        QMenuBar {
            background-color: #0F2636;
            border-bottom: 1px solid #27446d;
        }
        QMenuBar::item:selected {
            background-color: #1A3652;
        }

        /* Custom Title Bar ID */
        QWidget#customTitleBar {
            background-color: #0A1A26;
        }
    )";
    a.setStyleSheet(globalDarkStyle);
    qInstallMessageHandler(customMessageHandler);
    TDFManager::instance()->initializeTDFStructure();
    MainWindow w;
    w.show();
    return a.exec();
}

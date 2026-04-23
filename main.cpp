/* =============================================================================
 * FILE:         main.cpp
 * MODULE:       Application Entry Point
 * PROJECT:      Indigenous Scenario and Sensor Simulation Toolkit (ISSST)
 * ORGANISATION: Oxygen 2 Innovation (O2I).
 * STANDARD:     RTCA DO-178C / ED-12C, DAL B
 * COVERAGE:     Branch / Decision Coverage required (100% true/false paths)
 *
 * DESCRIPTION:  Application entry point – initialises QApplication, registers
 *               custom meta‑types, applies a dark theme stylesheet, installs
 *               a custom message handler for debug/log output, initialises the
 *               TDF (Tool Data Folder) structure, creates and shows the main
 *               window, and enters the Qt event loop.
 *
 * REQUIREMENTS: REQ-MAIN-030  Application initialisation
 *               REQ-MAIN-031  Register custom meta‑types for Qt signals/slots
 *               REQ-MAIN-032  Apply global dark theme stylesheet
 *               REQ-MAIN-033  Install custom message handler for logging
 *               REQ-MAIN-034  Initialise TDF folder structure at startup
 *               REQ-MAIN-035  Show main window and start event loop
 *
 * AUTHOR:       Arti Rajpoot
 * REVIEWED BY:  [Reviewer Name], [Review Date] — SPR-MAIN-002
 *
 *
 * COPYRIGHT:    Oxygen 2 Innovation (O2I). All rights reserved.
 *               Restricted circulation — defence simulation use only.
 * =============================================================================
 */
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

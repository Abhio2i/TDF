

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
    qInstallMessageHandler(customMessageHandler);
      TDFManager::instance()->initializeTDFStructure();
    MainWindow w;
    w.show();

    return a.exec();
}



// #include <QApplication>
// #include "GUI/Panel/ewdisplay.h"

// int main(int argc, char *argv[])
// {
//     QApplication app(argc, argv);
//     EWDisplay radar;
//     radar.show();
//     radar.raise();
//     radar.activateWindow();
//     return app.exec();
// }

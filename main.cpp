

#include "GUI/mainwindow.h"
#include <QApplication>
#include "core/Debug/console.h"

void customMessageHandler(QtMsgType type, const QMessageLogContext &context, const QString &msg);

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    qInstallMessageHandler(customMessageHandler);
    MainWindow w;
    w.show();

    return a.exec();
}

//gyuhfgjhfg

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

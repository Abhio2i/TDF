#include "./GUI/Editors/databaseeditor.h"
#include <QApplication>
#include "core/Debug/console.h"
void customMessageHandler(QtMsgType type, const QMessageLogContext &context, const QString &msg);
int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    qInstallMessageHandler(customMessageHandler);
    DatabaseEditor w;
    w.show();

    return a.exec();
}


// #include "./GUI/Editors/scenarioeditor.h"
// #include <QApplication>

// int main(int argc, char *argv[])
// {
//     QApplication a(argc, argv);

//     ScenarioEditor w;
//     w.show();

//     return a.exec();
// }

// //======================feedback===============
// #include "GUI/Feedback/feedback.h"
// #include <QApplication>
// int main(int argc, char *argv[])
// {
//     QApplication a(argc, argv);
//     qDebug() << "Creating Feedback window...";
//     Feedback w;
//     qDebug() << "Showing window...";
//     w.show();
//     qDebug() << "Entering event loop...";
//     return a.exec();
// }

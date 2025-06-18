
#include "console.h"
#include <iostream>
#include <QJsonDocument>
#include <QJsonObject>
#include <QDebug>
#include <QDateTime>

Console::Console() {
    logList = new std::unordered_map<std::string, std::string>();
}

Console* Console::internalInstance()
{
    static Console instance;
    return &instance;
}

void Console::log(std::string log) {
    std::cout << "Log: " << log << std::endl;
    internalInstance()->logList->insert({"log", log});
    emit internalInstance()->logUpdate(log);
}

void Console::log(QJsonObject obj) {
    QJsonDocument doc(obj);
    QString jsonString = doc.toJson(QJsonDocument::Indented);
    std::string log = jsonString.toStdString();
    std::cout << "Log: " << log << std::endl;
    internalInstance()->logList->insert({"log", log});
    emit internalInstance()->logUpdate(log);
}

void Console::error(std::string msg) {
    std::cerr << "Error: " << msg << std::endl;
    internalInstance()->logList->insert({"error", msg});
    emit internalInstance()->errorUpdate(msg);
}

void Console::warning(std::string warning) {
    std::clog << "Warning: " << warning << std::endl;
    internalInstance()->logList->insert({"warning", warning});
    emit internalInstance()->warningUpdate(warning);
}

void Console::saveLog() {
    // Implementation for saving logs (unchanged)
}

void Console::getLog() {
    // Implementation for retrieving logs (unchanged)
}

void Console::clearLog() {
    // Implementation for clearing logs (unchanged)
}

void customMessageHandler(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
    QString logLevel;

    switch (type) {
    case QtDebugMsg:
        logLevel = "[Debug]";
        break;
    case QtInfoMsg:
        logLevel = "[Info]";
        break;
    case QtWarningMsg:
        logLevel = "[Warning]";
        break;
    case QtCriticalMsg:
        logLevel = "[Critical]";
        break;
    case QtFatalMsg:
        logLevel = "[Fatal]";
        break;
    }

    QString finalMsg = QString("%1 %2 (%3:%4, %5): %6")
                           .arg(QDateTime::currentDateTime().toString("hh:mm:ss"))
                           .arg(logLevel)
                           .arg(context.file ? context.file : "")
                           .arg(context.line)
                           .arg(context.function ? context.function : "")
                           .arg(msg);

    // Send to Console singleton with appropriate signal
    switch (type) {
    case QtDebugMsg:
        Console::internalInstance()->debugUpdate(finalMsg.toStdString());
        break;
    case QtWarningMsg:
        Console::warning(finalMsg.toStdString());
        break;
    case QtCriticalMsg:
    case QtFatalMsg:
        Console::error(finalMsg.toStdString());
        break;
    case QtInfoMsg:
    default:
        Console::log(finalMsg.toStdString());
        break;
    }

    if (type == QtFatalMsg)
        abort();
}

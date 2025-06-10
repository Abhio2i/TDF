#ifndef CONSOLE_H
#define CONSOLE_H
#include <QObject>

class Console: public QObject
{
    Q_OBJECT
public:
    static Console* internalInstance();

    std::unordered_map<std::string, std::string> *logList;

    static void log(std::string log);
    static void log(QJsonObject obj);
    static void error(std::string error);
    static void warning(std::string warning);

    void saveLog();
    void getLog();
    void clearLog();
signals:
    void logUpdate(std::string log);

private:
    Console();
};

#endif // CONSOLE_H

#ifndef FRAME_H
#define FRAME_H
#include <QObject>
using namespace std;

struct ExcutesLog{
    int number;
    string startTime;
    string endTime;
    string excutionTime;
};

class Frame: public QObject
{
    Q_OBJECT
public:
    Frame();
    std::unordered_map<std::string,  ExcutesLog> *excutesLogs;
    int number;
    string startTime;
    string endTime;
    string excutionTime;
};

#endif // FRAME_H

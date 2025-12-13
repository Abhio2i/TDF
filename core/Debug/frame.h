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
    int excutionTime = 0;//ms
    int GUITime = 0;//ms
    int canvasTime = 0;//ms
    int physicsTime = 0;//ms
    int dynamicTime = 0;//ms
    int SensorTime = 0;//ms
    int RadarTime = 0;//ms
    int EWTime = 0;//ms
    int CSMTime = 0;//ms
    int ESMTime = 0;//ms
    int IFFTime = 0;//ms
    int RadioTime = 0;//ms
    int totalEntity = 0;
    int totalSensor = 0;
    int totalRadio = 0;
    int totalIff = 0;
    int csmdisplay = 0;
    int totaldynamicModel = 0;
    int esmdisplay = 0;

    void clean();
    void analyze();

};

#endif // FRAME_H

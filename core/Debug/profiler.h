#ifndef PROFILER_H
#define PROFILER_H
#include <QObject>
#include <core/Debug/frame.h>

class Profiler: public QObject
{
    Q_OBJECT
public:
    Profiler();
    Frame *currentFrame;
    std::unordered_map<std::string,  Frame> *frames;

    void addExecuteLogs();
};

#endif // PROFILER_H

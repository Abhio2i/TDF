#ifndef EOWETHER_H
#define EOWETHER_H

#include <QObject>
#include <QOpenGLFunctions>

class EOWether : public QObject, protected QOpenGLFunctions
{
    Q_OBJECT

public:
    explicit EOWether(QObject *parent = nullptr);
    ~EOWether() override;

    void initializeGLContext();
    void render();
};

#endif // EOWETHER_H

#ifndef IRWETHER_H
#define IRWETHER_H

#include <QObject>
#include <QOpenGLFunctions>

class IRWether : public QObject, protected QOpenGLFunctions
{
    Q_OBJECT

public:
    explicit IRWether(QObject *parent = nullptr);
    ~IRWether() override;

    void initializeGLContext();
    void render();
};

#endif // IRWETHER_H

#ifndef DATABASE_H
#define DATABASE_H

#include <QObject>
#include <core/Config/scenarioconfig.h>
#include <core/structure/hierarchy.h>
#include <core/Config/sessionmanager.h>
#include <core/Simulation/simulation.h>
#include <core/Render/scenerenderer.h>
#include <core/Network/networkmanager.h>
#include <core/Debug/console.h>

class Database: public QObject  // QObject se inherit kiya
{
    Q_OBJECT  // Meta-object system ke liye zaroori hai

public:
    Database();
    Hierarchy *hierarchy;
    Hierarchy *Mission;
    Console *console;
signals:

public slots:
};


#endif // DATABASE_H

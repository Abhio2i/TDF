#include "pluginmanager.h"
#include "core/Plugins/Interfaces/HierarchyInterface.h"
#include "core/Plugins/Interfaces/SensorInterface.h"
#include <QPluginLoader>
#include <QDebug>

PluginManager::PluginManager() {

}

bool PluginManager::loadPlugin(std::string path){
    qDebug() << "[PluginManager] loadPlugin CALLED | path:"
             << QString::fromStdString(path);

    // Linux par shared libraries ke aage 'lib' aur piche '.so' lagta hai
    QString pluginPath = "/home/o2i/plugin/build/Desktop_Qt_5_15_2_GCC_64bit-Debug/libplugin.so";
    if(!path.empty()){
        pluginPath  = QString::fromStdString(path);
    }
    // 1. Plugin Load karo
    QPluginLoader* loader = new QPluginLoader(pluginPath);
    bool loaded = loader->load();
    QObject *instance = loader->instance();
    if (instance) {
        // 2. Class ko convert (cast) karo hamare Interface mein
        HierarchyInterface *hierInterface = qobject_cast<HierarchyInterface*>(instance);
        SensorInterface *sensorInterface = qobject_cast<SensorInterface*>(instance);

        if (hierInterface) {
            pluginList[path] = loader;

            if(hierarchy){
                // connect syntax with Lambda function
                QMetaObject::Connection conn1 = connect(hierarchy, &Hierarchy::entityAdded, this, [hierInterface](QString parentID, QString ID, QString entityName) {
                    hierInterface->entityAdded(ID.toStdString(),"Platform");
                });
                pluginconnection[path].push_back(conn1);

                QMetaObject::Connection conn2 = connect(hierarchy, &Hierarchy::entityRemoved, this, [hierInterface](QString ID) {
                    hierInterface->entityRemoved(ID.toStdString());
                });
                pluginconnection[path].push_back(conn2);

                QMetaObject::Connection conn3 = connect(hierarchy, &Hierarchy::entiitySelected, this, [hierInterface](QString ID) {
                    hierInterface->entitySelected(ID.toStdString());
                });
                pluginconnection[path].push_back(conn3);

                hierInterface->registerCreateEntityCallback([this](std::string parentID,std::string name) -> bool {
                    return hierarchy->addEntity(QString::fromStdString(parentID),QString::fromStdString(name),true);
                });

                hierInterface->registerRenameEntityCallback([this](std::string Id,std::string name) -> bool {
                    hierarchy->renameEntity(QString::fromStdString(Id),QString::fromStdString(name));
                    return true;
                });

                hierInterface->registerRemoveEntityCallback([this](std::string parentID,std::string ID) -> bool {
                    hierarchy->removeEntity(QString::fromStdString(parentID),QString::fromStdString(ID));
                    return true;
                });

                hierInterface->registerGetEntityByTypeCallback([this](std::string prop) -> std::vector<std::string> {
                    std::vector<std::string> entityDataList;
                    for (const auto& [key, entityPtr] : hierarchy->Platforms) {
                        if (entityPtr) {
                            entityDataList.push_back(key);
                        }
                    }
                    return entityDataList; // Pure vector ko return kiya
                });

                hierInterface->registerGetEntityGeoDataByIDCallback([this](std::string id) -> HierarchyInterface::geoData {
                    HierarchyInterface::geoData geodata;
                    auto it = hierarchy->Platforms.find(id);
                    if (it != hierarchy->Platforms.end()) {
                        if(it->second->transform){
                            geodata.lat = it->second->transform->getLatitude();
                            geodata.lon = it->second->transform->getLongitude();
                            geodata.alt = it->second->transform->getAltitude();
                            geodata.head = it->second->transform->getHeading();
                            geodata.pitch = it->second->transform->pitch();
                            geodata.roll = it->second->transform->roll();
                            geodata.yaw = it->second->transform->yaw();
                        }

                    }
                    return geodata; // Pure vector ko return kiya
                });

                hierInterface->registerGetEntityTransformByIDCallback([this](std::string id) -> HierarchyInterface::transformData {
                    HierarchyInterface::transformData transformData;
                    auto it = hierarchy->Platforms.find(id);
                    if (it != hierarchy->Platforms.end()) {
                        if(it->second->transform){
                            QVector3D pos = it->second->transform->translation();
                            transformData.x = pos.x();
                            transformData.y = pos.y();
                            transformData.z = pos.z();

                            QQuaternion rot = it->second->transform->rotation();
                            transformData.rotX = rot.x();
                            transformData.rotY = rot.y();
                            transformData.rotZ = rot.z();
                            transformData.rotW = rot.scalar();

                            QVector3D scale = it->second->transform->scale3D();
                            transformData.scaleX = scale.x();
                            transformData.scaleY = scale.y();
                            transformData.scaleZ = scale.z();
                        }

                    }
                    return transformData; // Pure vector ko return kiya
                });
            }
            if(simulation){
                QMetaObject::Connection conn1 = connect(simulation, &Simulation::Render, this, [hierInterface](float delta) {
                    hierInterface->update(delta);
                });
                pluginconnection[path].push_back(conn1);
            }
        }else
        if(sensorInterface){
            pluginList[path] = loader;
        }
        else {
            qDebug() << "Interface match nahi hua!";
            return false;
        }
    } else {
        std::cout << "Linux par plugin load nahi hua. Reason:" << loader->errorString().toStdString();
        return false;
    }
    return true;
}

bool PluginManager::unloadPlugin(std::string path){
    std::string key = path;
    auto it = pluginList.find(key);

    if (it != pluginList.end()) {
        QPluginLoader* loader = it->second;
        bool loaded = loader->isLoaded();
        if (loader && loaded) {
            QObject* instance = loader->instance();
            if (pluginconnection.find(key) != pluginconnection.end())
            {
                // Vector ke har connection par loop chalayein
                for (const QMetaObject::Connection &conn : pluginconnection[key])
                {
                    QObject::disconnect(conn);
                }

                // Connections todne ke baad vector ko clear kar dein ya key uda dein
                pluginconnection.erase(key);
                qDebug() << "All connections for" << QString::fromStdString(key) << "disconnected successfully!";
            }

            if (instance) {
                delete instance;
            }

            // Ab binary ko unload karein
            loader->unload();
            // Heap object ko delete karein aur map se nikalein
            // delete loader;
        }
        pluginList.erase(it);
        return true;
    }
    return false;
}

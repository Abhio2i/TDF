#include "pluginmanager.h"
#include "core/Plugins/Interfaces/HierarchyInterface.h"
#include "core/Plugins/Interfaces/customIFFInterface.h"
#include "core/Plugins/Interfaces/customRadioInterface.h"
#include "core/Plugins/Interfaces/customSensorInterface.h"
#include "core/Plugins/Interfaces/aesaRadarInterface.h"
#include "core/Plugins/aesalogicregistry.h"
#include "core/Hierarchy/EntityProfiles/SensorProfiles/aesaradar.h"
#include <memory>
#include <unordered_set>
#include "core/Plugins/Interfaces/uitoolbarinterface.h"
#include "core/Plugins/Interfaces/uigisinterface.h"   // ← YE ADD KARO

#include "GIS_API/backend/shapes/shape_type_registry.h"
#include "GIS_API/backend/formats/layer_format_registry.h"
#include "GIS_API/backend/api/map_api.h"
#include "GIS_API/backend/api/shape_api.h"
#include "GIS_API/frontend/ui_action_api.h"

#include <QPluginLoader>
#include <QDebug>
#include <iostream>

PluginManager::PluginManager() {
}

// =============================================================================
// AESA plugin SDK helpers. REQ-AESA-090.
// =============================================================================
static AESARadar* findAesaRadar(Hierarchy* hier, const std::string& id)
{
    if (hier == nullptr) return nullptr;
    auto it = hier->Sensors.find(id);
    return (it != hier->Sensors.end()) ? dynamic_cast<AESARadar*>(it->second) : nullptr;
}

static aesaplugin::RadarInfo aesaRadarInfo(const std::string& id, const AESARadar* r)
{
    aesaplugin::RadarInfo info;
    info.radarId  = id;
    info.entityId = (r->parentEntity != nullptr) ? r->parentEntity->ID : std::string();
    info.name     = r->Name;
    return info;
}

bool PluginManager::loadPlugin(std::string path)
{
    qDebug() << "[PluginManager] loadPlugin CALLED | path:"
             << QString::fromStdString(path);

    QString pluginPath = "/home/o2i/plugin/build/Desktop_Qt_5_15_2_GCC_64bit-Debug/libplugin.so";
    if (!path.empty()) {
        pluginPath = QString::fromStdString(path);
    }

    QPluginLoader* loader = new QPluginLoader(pluginPath);
    bool loaded = loader->load();
    QObject *instance = loader->instance();

    qDebug() << "[PluginManager] loaded=" << loaded
             << " instance=" << (void*)instance
             << " err=" << loader->errorString();

    if (!(instance && loaded)) {
        qDebug() << "Plugin load FAILED";
        delete loader;
        return false;
    }
    // ============================================================
    // 1. HierarchyInterface
    // ============================================================
    HierarchyInterface* hierInterface = qobject_cast<HierarchyInterface*>(instance);
    if (hierInterface) {
        pluginList[path] = loader;

        if (hierarchy) {
            QMetaObject::Connection conn1 = connect(hierarchy, &Hierarchy::entityAdded, this,
                                                    [hierInterface](QString parentID, QString ID, QString entityName) {
                                                        Q_UNUSED(parentID); Q_UNUSED(entityName);
                                                        hierInterface->entityAdded(ID.toStdString(), "Platform");
                                                    });
            pluginconnection[path].push_back(conn1);

            QMetaObject::Connection conn2 = connect(hierarchy, &Hierarchy::entityRemoved, this,
                                                    [hierInterface](QString ID) {
                                                        hierInterface->entityRemoved(ID.toStdString());
                                                    });
            pluginconnection[path].push_back(conn2);

            QMetaObject::Connection conn3 = connect(hierarchy, &Hierarchy::entiitySelected, this,
                                                    [hierInterface](QString ID) {
                                                        hierInterface->entitySelected(ID.toStdString());
                                                    });
            pluginconnection[path].push_back(conn3);

            hierInterface->registerCreateEntityCallback([this](std::string parentID, std::string name) -> bool {
                return hierarchy->addEntity(QString::fromStdString(parentID),
                                            QString::fromStdString(name), true);
            });

            hierInterface->registerRenameEntityCallback([this](std::string Id, std::string name) -> bool {
                hierarchy->renameEntity(QString::fromStdString(Id), QString::fromStdString(name));
                return true;
            });

            hierInterface->registerRemoveEntityCallback([this](std::string parentID, std::string ID) -> bool {
                hierarchy->removeEntity(QString::fromStdString(parentID), QString::fromStdString(ID));
                return true;
            });

            hierInterface->registerGetEntityByTypeCallback([this](std::string prop) -> std::vector<std::string> {
                Q_UNUSED(prop);
                std::vector<std::string> entityDataList;
                for (const auto& [key, entityPtr] : hierarchy->Platforms) {
                    if (entityPtr) entityDataList.push_back(key);
                }
                return entityDataList;
            });

            hierInterface->registerGetEntityGeoDataByIDCallback([this](std::string id) -> HierarchyInterface::geoData {
                HierarchyInterface::geoData geodata;
                auto it = hierarchy->Platforms.find(id);
                if (it != hierarchy->Platforms.end()) {
                    geodata.lat   = it->second->transform.getLatitude();
                    geodata.lon   = it->second->transform.getLongitude();
                    geodata.alt   = it->second->transform.getAltitude();
                    geodata.head  = it->second->transform.getHeading();
                    geodata.pitch = it->second->transform.pitch();
                    geodata.roll  = it->second->transform.roll();
                    geodata.yaw   = it->second->transform.yaw();
                }
                return geodata;
            });

            hierInterface->registerGetEntityTransformByIDCallback([this](std::string id) -> HierarchyInterface::transformData {
                HierarchyInterface::transformData transformData;
                auto it = hierarchy->Platforms.find(id);
                if (it != hierarchy->Platforms.end()) {
                    QVector3D pos = it->second->transform.translation();
                    transformData.x = pos.x(); transformData.y = pos.y(); transformData.z = pos.z();
                    QQuaternion rot = it->second->transform.rotation();
                    transformData.rotX = rot.x(); transformData.rotY = rot.y();
                    transformData.rotZ = rot.z(); transformData.rotW = rot.scalar();
                    QVector3D scale = it->second->transform.scale3D();
                    transformData.scaleX = scale.x(); transformData.scaleY = scale.y(); transformData.scaleZ = scale.z();
                }
                return transformData;
            });
        }

        if (simulation) {
            QMetaObject::Connection conn1 = connect(simulation, &Simulation::Render, this,
                                                    [hierInterface](float delta) {
                                                        hierInterface->update(delta);
                                                    });
            pluginconnection[path].push_back(conn1);
        }

        qDebug() << "[PluginManager] HierarchyInterface loaded";
        return true;
    }

    // ============================================================
    // 2. SensorFactoryInterface
    // ============================================================
    SensorFactoryInterface* sensorFactoryInterface = qobject_cast<SensorFactoryInterface*>(instance);
    if (sensorFactoryInterface) {
        pluginList[path] = loader;
        qDebug() << "Plugin loaded successfully. Creating instances...";

        if (hierarchy) {
            QMetaObject::Connection conn3 = connect(hierarchy, &Hierarchy::entiitySelected, this,
                                                    [sensorFactoryInterface](QString ID) {
                                                        sensorFactoryInterface->selectedEntity(ID);
                                                    });
            pluginconnection[path].push_back(conn3);

            QMetaObject::Connection conn4 = connect(hierarchy, &Hierarchy::getCustomSensor, this,
                                                    [sensorFactoryInterface](QString entityID, uint32_t ecsID, QString sensorID, CustomSensorInterface*& sensor) {
                                                        sensor = sensorFactoryInterface->createSensor(entityID, ecsID, sensorID);
                                                    });
            pluginconnection[path].push_back(conn4);

            sensorFactoryInterface->registerNewSensorCallback([this](std::string prop) -> bool {
                return hierarchy->registerNewSensor(prop);
            });

            sensorFactoryInterface->registerRemoveNewSensorCallback([this](std::string prop) -> bool {
                return hierarchy->unregisterNewSensor(prop);
            });

            sensorFactoryInterface->registerSetSensorPanelCallback([this](QWidget* panel) -> bool {
                emit uiPluginManager.registerSensorPanel(panel);
                return true;
            });

            sensorFactoryInterface->init(&hierarchy->world);
        }

        if (simulation) {
            QMetaObject::Connection conn1 = connect(simulation, &Simulation::Render, this,
                                                    [sensorFactoryInterface](float /*delta*/) {
                                                        sensorFactoryInterface->update();
                                                    });
            pluginconnection[path].push_back(conn1);
        }

        qDebug() << "[PluginManager] SensorFactoryInterface loaded";
        return true;
    }

    RadioFactoryInterface* radioFactoryInterface = qobject_cast<RadioFactoryInterface*>(instance);
    if (radioFactoryInterface) {
        pluginList[path] = loader;
        qDebug() << "Plugin loaded successfully. Creating instances...";

        if (hierarchy) {
            QMetaObject::Connection conn3 = connect(hierarchy, &Hierarchy::entiitySelected, this,
                                                    [radioFactoryInterface](QString ID) {
                                                        radioFactoryInterface->selectedEntity(ID);
                                                    });
            pluginconnection[path].push_back(conn3);

            QMetaObject::Connection conn4 = connect(hierarchy, &Hierarchy::getCustomRadio, this,
                                                    [radioFactoryInterface](QString entityID, uint32_t ecsID, QString radioID, CustomRadioInterface*& radio) {
                                                        radio = radioFactoryInterface->createRadio(entityID, ecsID, radioID);
                                                    });
            pluginconnection[path].push_back(conn4);

            radioFactoryInterface->registerNewRadioCallback([this](std::string prop) -> bool {
                return hierarchy->registerNewRadio(prop);
            });

            radioFactoryInterface->registerRemoveNewRadioCallback([this](std::string prop) -> bool {
                return hierarchy->unregisterNewRadio(prop);
            });

            radioFactoryInterface->registerSetRadioPanelCallback([this](QWidget* panel) -> bool {
                // emit uiPluginManager.registerRadioPanel(panel);
                return true;
            });

            radioFactoryInterface->init(&hierarchy->world);
        }

        if (simulation) {
            QMetaObject::Connection conn1 = connect(simulation, &Simulation::Render, this,
                                                    [radioFactoryInterface](float /*delta*/) {
                                                        radioFactoryInterface->update();
                                                    });
            pluginconnection[path].push_back(conn1);
        }

        qDebug() << "[PluginManager] radioFactoryInterface loaded";
        return true;
    }

    IFFFactoryInterface* iffFactoryInterface = qobject_cast<IFFFactoryInterface*>(instance);
    if (iffFactoryInterface) {
        pluginList[path] = loader;
        qDebug() << "Plugin loaded successfully. Creating instances...";

        if (hierarchy) {
            QMetaObject::Connection conn3 = connect(hierarchy, &Hierarchy::entiitySelected, this,
                                                    [iffFactoryInterface](QString ID) {
                                                        iffFactoryInterface->selectedEntity(ID);
                                                    });
            pluginconnection[path].push_back(conn3);

            QMetaObject::Connection conn4 = connect(hierarchy, &Hierarchy::getCustomIFF, this,
                                                    [iffFactoryInterface](QString entityID, uint32_t ecsID, QString iffID, CustomIFFInterface*& iff) {
                                                        iff = iffFactoryInterface->createIFF(entityID, ecsID, iffID);
                                                    });
            pluginconnection[path].push_back(conn4);

            iffFactoryInterface->registerNewIFFCallback([this](std::string prop) -> bool {
                return hierarchy->registerNewIFF(prop);
            });

            iffFactoryInterface->registerRemoveNewIFFCallback([this](std::string prop) -> bool {
                return hierarchy->unregisterNewIFF(prop);
            });

            iffFactoryInterface->registerSetIFFPanelCallback([this](QWidget* panel) -> bool {
                // emit uiPluginManager.registerRadioPanel(panel);
                return true;
            });

            iffFactoryInterface->init(&hierarchy->world);
        }

        if (simulation) {
            QMetaObject::Connection conn1 = connect(simulation, &Simulation::Render, this,
                                                    [iffFactoryInterface](float /*delta*/) {
                                                        iffFactoryInterface->update();
                                                    });
            pluginconnection[path].push_back(conn1);
        }

        qDebug() << "[PluginManager] iffFactoryInterface loaded";
        return true;
    }

    // ============================================================
    // AesaRadarPluginInterface (AESA plugin SDK, REQ-AESA-090)
    // ============================================================
    AesaRadarPluginInterface* aesaIface = qobject_cast<AesaRadarPluginInterface*>(instance);
    if (aesaIface) {
        if (aesaIface->apiVersion() != aesaplugin::API_VERSION) {
            qWarning() << "[PluginManager] AESA plugin API version" << aesaIface->apiVersion()
                       << "does not match host version" << aesaplugin::API_VERSION;
            loader->unload();
            delete loader;
            return false;
        }
        if (!hierarchy) {
            qWarning() << "[PluginManager] AESA plugin needs a hierarchy";
            loader->unload();
            delete loader;
            return false;
        }
        pluginList[path] = loader;
        Hierarchy* hier = hierarchy;

        // ---- Host callbacks. All refuse while a plugin hook is running. ----
        aesaIface->registerListRadarsCallback([hier]() {
            std::vector<aesaplugin::RadarInfo> out;
            if (AesaLogicRegistry::inStage()) return out;
            for (const auto& [id, sensor] : hier->Sensors) {
                if (AESARadar* r = dynamic_cast<AESARadar*>(sensor))
                    out.push_back(aesaRadarInfo(id, r));
            }
            return out;
        });

        aesaIface->registerGetConfigCallback([hier](const std::string& radarId) -> QJsonObject {
            if (AesaLogicRegistry::inStage()) return QJsonObject();
            AESARadar* r = findAesaRadar(hier, radarId);
            return r ? r->toJson() : QJsonObject();
        });

        aesaIface->registerSetConfigCallback([hier](const std::string& radarId,
                                                    const QJsonObject& partial) -> bool {
            if (AesaLogicRegistry::inStage()) return false;
            AESARadar* r = findAesaRadar(hier, radarId);
            if (!r) return false;
            r->fromJson(partial);
            emit hier->sensorUpdated(QString::fromStdString(radarId));
            return true;
        });

        aesaIface->registerGetStatusCallback([hier](const std::string& radarId) {
            aesaplugin::RadarStatus st;
            if (AesaLogicRegistry::inStage()) return st;
            AESARadar* r = findAesaRadar(hier, radarId);
            if (!r) return st;
            st.valid = true;
            switch (r->getMode()) {
            case aesa::RadarMode::SURVEILLANCE: st.mode = 0; break;
            case aesa::RadarMode::TWS:          st.mode = 1; break;
            case aesa::RadarMode::LOCK_ON:      st.mode = 2; break;
            }
            st.beamAzimuth_deg   = r->getCurrentAzimuth();
            st.beamElevation_deg = r->getCurrentElevation();
            st.dutyCycle         = r->getCurrentDutyCycle();
            st.displayRange_km   = r->range;
            return st;
        });

        aesaIface->registerSetLogicHooksCallback([path](const aesaplugin::LogicHooks& hooks) -> bool {
            if (AesaLogicRegistry::inStage()) return false;
            std::string err;
            const bool ok = AesaLogicRegistry::instance().setHooks(path, hooks, &err);
            if (!ok)
                qWarning() << "[PluginManager] AESA hooks rejected:" << QString::fromStdString(err);
            return ok;
        });

        aesaIface->registerClearLogicHooksCallback([path]() {
            if (AesaLogicRegistry::inStage()) return;
            AesaLogicRegistry::instance().removeOwner(path);
        });

        // ---- Radar add/remove notifications. ----
        auto known = std::make_shared<std::unordered_set<std::string>>();

        QMetaObject::Connection connAdd = connect(hierarchy, &Hierarchy::sensorAdded, this,
            [hier, aesaIface, known](const QString& /*entityId*/, const QString& sensorId) {
                const std::string id = sensorId.toStdString();
                AESARadar* r = findAesaRadar(hier, id);
                if (r && known->insert(id).second)
                    aesaIface->radarAdded(aesaRadarInfo(id, r));
            });
        pluginconnection[path].push_back(connAdd);

        QMetaObject::Connection connRemove = connect(hierarchy, &Hierarchy::sensorRemoved, this,
            [aesaIface, known](const QString& /*entityId*/, const QString& sensorId) {
                const std::string id = sensorId.toStdString();
                if (known->erase(id) > 0)
                    aesaIface->radarRemoved(id);
            });
        pluginconnection[path].push_back(connRemove);

        if (simulation) {
            QMetaObject::Connection connRender = connect(simulation, &Simulation::Render, this,
                [aesaIface](float delta) {
                    aesaIface->update(delta);
                });
            pluginconnection[path].push_back(connRender);
        }

        aesaIface->init();

        // Announce radars that already exist.
        for (const auto& [id, sensor] : hier->Sensors) {
            if (AESARadar* r = dynamic_cast<AESARadar*>(sensor)) {
                if (known->insert(id).second)
                    aesaIface->radarAdded(aesaRadarInfo(id, r));
            }
        }

        qDebug() << "[PluginManager] AesaRadarPluginInterface loaded";
        return true;
    }

    // ============================================================
    // 3. UIToolbarInterface
    // ============================================================
    UIToolbarInterface* uiToolbar = qobject_cast<UIToolbarInterface*>(instance);
    if (uiToolbar) {
        pluginList[path] = loader;

        uiToolbar->registerDesignActionCallback([this](QAction* action, const QString& pluginId) {
            emit uiPluginManager.registerDesignToolbarAction(action, pluginId);
        });

        uiToolbar->registerRuntimeActionCallback([this](QAction* action, const QString& pluginId) {
            emit uiPluginManager.registerRuntimeToolbarAction(action, pluginId);
        });

        uiToolbar->registerNetworkActionCallback([this](QAction* action, const QString& pluginId) {
            emit uiPluginManager.registerNetworkToolbarAction(action, pluginId);
        });

        uiToolbar->registerMenuActionCallback([this](const QString& menuName, QAction* action, const QString& pluginId) {
            emit uiPluginManager.registerMenuAction(menuName, action, pluginId);
        });

        uiToolbar->initializeUI();
        qDebug() << "[PluginManager] UIToolbarInterface loaded";
        return true;
    }

    // ============================================================
    // 4. UIGISInterface  (SEPARATE block)
    // ============================================================

    UIGISInterface* uiGis = qobject_cast<UIGISInterface*>(instance);
    if (uiGis) {
        pluginList[path] = loader;
        m_gisRegs[path] = GisPluginRegistrations{};  // track for unload

        // 1. Toolbar action
        uiGis->setRegisterGisAction(
            [this, path](const QString& id, const QString& label, const QString& iconPath,
                         const QString& category, bool checkable, int priority,
                         std::function<void(const UiActionContext&)> onTriggered)
            {
                UiActionDescriptor d;
                d.id        = id;
                d.label     = label;
                d.iconPath  = iconPath;
                d.checkable = checkable;
                d.priority  = priority;
                d.callback  = [onTriggered](const UiActionContext& ctx) {
                    if (onTriggered)
                        onTriggered(ctx);
                };

                if      (category == "ToolbarMain")     d.category = UiActionCategory::ToolbarMain;
                else if (category == "ToolbarShape")    d.category = UiActionCategory::ToolbarShape;
                else if (category == "ToolbarBitmap")   d.category = UiActionCategory::ToolbarBitmap;
                else if (category == "ToolbarPreset")   d.category = UiActionCategory::ToolbarPreset;
                else if (category == "ToolbarMapLayer") d.category = UiActionCategory::ToolbarMapLayer;
                else if (category == "ToolbarCoord")    d.category = UiActionCategory::ToolbarCoord;
                else                                    d.category = UiActionCategory::Custom;

                UiActionApi::instance()->registerAction(d);
                m_gisRegs[path].actionIds.append(id);
            });

        // 2. Context menu
        uiGis->setRegisterGisContext(
            [this, path](const QString& id, const QString& label, const QString& contextType,
                         const QString& group, int priority,
                         std::function<void(const QString&, const QString&)> onTriggered)
            {
                UiActionDescriptor d;
                d.id       = id;
                d.label    = label;
                d.group    = group;
                d.priority = priority;
                d.callback = [onTriggered](const UiActionContext& ctx) {
                    if (onTriggered) onTriggered(ctx.shapeId, ctx.layerId);
                };

                if      (contextType == "ContextMap")   d.category = UiActionCategory::ContextMap;
                else if (contextType == "ContextShape") d.category = UiActionCategory::ContextShape;
                else if (contextType == "ContextLayer") d.category = UiActionCategory::ContextLayer;
                else                                    d.category = UiActionCategory::Custom;

                UiActionApi::instance()->registerAction(d);
                m_gisRegs[path].actionIds.append(id);
            });

        // 3. Shape type
        uiGis->setRegisterShapeType([this, path](IShapeType* type) {
            if (!type)
                return;
            ShapeTypeRegistry::instance()->registerType(type);
            const QString tid = type->typeId();
            if (!tid.isEmpty())
                m_gisRegs[path].shapeTypeIds.append(tid);
        });

        // 4. Layer format
        uiGis->setRegisterLayerFormat([](ILayerFormat* fmt) {
            if (fmt)
                LayerFormatRegistry::instance()->registerFormat(fmt);
        });

        // 5. Tile source
        uiGis->setRegisterTileSource(
            [this, path](const QString& id, const QString& folder, const QString& name,
                         const QString& icon, int prio)
            {
                MapApi::instance()->registerTileSource(id, folder, name, icon, prio);
                m_gisRegs[path].tileSourceIds.append(id);
            });

        // 6. Host helpers
        uiGis->setMapCenter = [](double lat, double lon) {
            MapApi::instance()->setCenter(lat, lon);
        };
        uiGis->setMapZoom = [](int level) {
            MapApi::instance()->setZoom(level);
        };
        uiGis->addShape = [](const QJsonObject& data) -> QString {
            return ShapeApi::instance()->addShape(data);
        };
        uiGis->startDrawingShape = [](const QString& /*shapeType*/) {
        };

        uiGis->initializeGIS();
        qDebug() << "[PluginManager] UIGISInterface loaded"
                 << "actions=" << m_gisRegs[path].actionIds.size()
                 << "shapes=" << m_gisRegs[path].shapeTypeIds.size();
        return true;
    }
    // ============================================================
    // No interface matched
    // ============================================================
    qDebug() << "Interface match nahi hua!";
    loader->unload();
    delete loader;
    return false;
}

bool PluginManager::unloadPlugin(std::string path)
{
    std::string key = path;
    auto it = pluginList.find(key);

    if (it != pluginList.end()) {
        QPluginLoader* loader = it->second;
        if (loader && loader->isLoaded()) {
            if (pluginconnection.find(key) != pluginconnection.end()) {
                for (const QMetaObject::Connection& conn : pluginconnection[key]) {
                    QObject::disconnect(conn);
                }
                pluginconnection.erase(key);
                qDebug() << "All connections for" << QString::fromStdString(key)
                         << "disconnected successfully!";
            }

            auto regIt = m_gisRegs.find(key);
            if (regIt != m_gisRegs.end()) {
                const GisPluginRegistrations& regs = regIt->second;
                for (const QString& id : regs.actionIds) {
                    UiActionApi::instance()->unregisterAction(id);
                    qDebug() << "[PluginManager] unregistered action" << id;
                }
                for (const QString& id : regs.shapeTypeIds) {
                    ShapeTypeRegistry::instance()->unregisterType(id);
                    qDebug() << "[PluginManager] unregistered shape type" << id;
                }
                for (const QString& id : regs.layerFormatIds) {
                    LayerFormatRegistry::instance()->unregisterFormat(id);
                    qDebug() << "[PluginManager] unregistered layer format" << id;
                }
                for (const QString& id : regs.tileSourceIds) {
                    MapApi::instance()->unregisterTileSource(id);
                    qDebug() << "[PluginManager] unregistered tile source" << id;
                }
                m_gisRegs.erase(regIt);
            }

            QObject* instance = loader->instance();
            if (instance) {
                if (UIGISInterface* gis = qobject_cast<UIGISInterface*>(instance)) {
                    gis->unloadGIS();
                }
                if (UIToolbarInterface* tb = qobject_cast<UIToolbarInterface*>(instance)) {
                    tb->unloadUI();
                }
                if (AesaRadarPluginInterface* aesa = qobject_cast<AesaRadarPluginInterface*>(instance)) {
                    aesa->shutdown();
                }
            }

            // Remove any AESA logic hooks owned by this plugin BEFORE its code
            // is unloaded (a radar must never call into an unloaded library).
            // REQ-AESA-090.
            AesaLogicRegistry::instance().removeOwner(key);

            loader->unload();
        }
        pluginList.erase(it);
        return true;
    }
    return false;
}

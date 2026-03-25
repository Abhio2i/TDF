#include "runtime.h"
#include <QDebug>
#include <QThread>
#include <core/Network/libs/TransformUpdate.h> //for qued connnection
#include <QMetaType>//by aman
#include <core/SharedMemory/sharedmemorywrapper.h>
Runtime::Runtime() {
    qRegisterMetaType<TransformUpdate>("TransformUpdate");//by Aman

    // New By Himanshu Start
    // Register the Enum so it can be passed across threads via Signals/Slots
    qRegisterMetaType<SimulationStateNS::State>("SimulationStateNS::State");

    simulationThread = new QThread(this);
    Library = new Hierarchy();
    hierarchy = new Hierarchy();
    Library->fixedProfiles = false;
    // Setup Shared Memory Thread By Himanshu
    sharedMemoryThread = new QThread(this);
    sharedWrapper = new SharedMemoryWrapper();
    // Create SharedMemory. IMPORTANT: pass nullptr as parent!
    // If you pass 'this', you cannot move it to another thread.
    // sharedmemory = new SharedMemory(hierarchy, simulation, nullptr);

    // // Move SharedMemory to its thread
    // sharedmemory->moveToThread(sharedMemoryThread);

    // // Connect Thread Lifecycle
    // connect(sharedMemoryThread, &QThread::finished, sharedmemory, &QObject::deleteLater);
    // connect(sharedMemoryThread, &QThread::finished, sharedMemoryThread, &QObject::deleteLater);

    // // Start SharedMemory Thread
    // sharedMemoryThread->start();

    sqlite = new SQLite(hierarchy, simulation);//SQLite Memory By Himanshu

    scenerenderer = new SceneRenderer();
    simulation = new Simulation();
    scriptengine = new ScriptEngine();
     scriptengine->setRuntime(this);
    networkManager = new NetworkManager();
     simulation->setNetworkManager(networkManager);

    console  = Console::internalInstance();
    profiler = new Profiler();
    recorder = new Recorder(hierarchy, simulation);
    simulation->init();
    // simulation->moveToThread(simulationThread);

    // QObject::connect(simulationThread, &QThread::started, simulation, &Simulation::init);
    // connect(simulationThread,&QThread::finished,simulationThread,&QThread::deleteLater);
    // simulationThread->start();

    //fix:the hirarchy updated now only touched by simualtion thread to avoid race condition
    connect(networkManager,
            &NetworkManager::transformReceived,
            simulation,
            &Simulation::enqueueTransformUpdate,
            Qt::QueuedConnection);

    // Rendering pipeline
    connect(hierarchy,&Hierarchy::entityMeshAdded,scenerenderer,&SceneRenderer::entityAdded);
    connect(hierarchy,&Hierarchy::entityMeshRemoved,scenerenderer,&SceneRenderer::entityRemoved);
    connect(simulation,&Simulation::Render,scenerenderer,&SceneRenderer::Render);

    //SharedMemory Connection
    connect(hierarchy,&Hierarchy::entityPhysicsAdded,sharedWrapper,&SharedMemoryWrapper::entityAdded);
    connect(hierarchy,&Hierarchy::entityRemoved,sharedWrapper,&SharedMemoryWrapper::entityRemoved);
    connect(simulation,&Simulation::Render,sharedWrapper,&SharedMemoryWrapper::SimulationUpdate);

    // Physics system connections
    connect(hierarchy,&Hierarchy::Init,simulation,&Simulation::Reset);
    connect(hierarchy,&Hierarchy::entityPhysicsAdded,simulation,&Simulation::entityAdded);
    connect(hierarchy,&Hierarchy::entityRemoved,simulation,&Simulation::entityRemoved);
    connect(hierarchy,&Hierarchy::entityPhysicsRemoved,simulation,&Simulation::entityRemoved);
    connect(hierarchy,&Hierarchy::entityUpdate,simulation,&Simulation::entityUpdate);

    // NetworkManager connections
    networkManager->setHierarchy(hierarchy);

    connect(hierarchy, &Hierarchy::folderAdded,
            networkManager, &NetworkManager::folderAdded);
    connect(hierarchy, &Hierarchy::folderRemoved,
            networkManager, &NetworkManager::folderRemoved);

    connect(hierarchy, &Hierarchy::entityAddedPointer,
            networkManager, &NetworkManager::entityAddedPointer);
    connect(hierarchy, &Hierarchy::entityAdded,
            networkManager, &NetworkManager::entityAdded);
    connect(hierarchy, &Hierarchy::entityRemovedfull,
            networkManager, &NetworkManager::entityRemoved);
    connect(hierarchy, &Hierarchy::entityRenamed,
            networkManager, &NetworkManager::entityRenamed);
    connect(hierarchy, &Hierarchy::entityUpdate,
            networkManager, &NetworkManager::entityUpdate);

    connect(hierarchy, &Hierarchy::componentAdded,
            networkManager, &NetworkManager::componentAdded);

    connect(hierarchy, &Hierarchy::entityComponentUpdate,
            networkManager, &NetworkManager::entityComponentsUpdate);

    connect(hierarchy, &Hierarchy::entityMeshAdded,
            networkManager, &NetworkManager::entityMeshAdded);
    connect(hierarchy, &Hierarchy::entityMeshRemoved,
            networkManager, &NetworkManager::entityMeshRemoved);

    connect(hierarchy, &Hierarchy::entityPhysicsAdded,
            networkManager, &NetworkManager::entityPhysicsAdded);
    connect(hierarchy, &Hierarchy::entityPhysicsRemoved,
            networkManager, &NetworkManager::entityPhysicsRemoved);

    connect(simulation, &Simulation::Update,
            networkManager, &NetworkManager::UpdateClient);

    connect(networkManager,&NetworkManager::initData,hierarchy,&Hierarchy::fromJson);
    connect(networkManager,&NetworkManager::getCurrentJsonData,hierarchy,&Hierarchy::getCurrentJsonData);
    connect(hierarchy,&Hierarchy::getJsonData,networkManager,&NetworkManager::getJsonData);
    connect(networkManager,&NetworkManager::addFolder,hierarchy,&Hierarchy::addFolderViaNetwork);
    connect(networkManager,&NetworkManager::removeFolder,hierarchy,&Hierarchy::removeFolderViaNetwork);
    connect(networkManager,&NetworkManager::addEntity,hierarchy,&Hierarchy::addEntityViaNetwork);
    connect(networkManager,&NetworkManager::addComponent,hierarchy,&Hierarchy::addComponent);
    connect(networkManager,&NetworkManager::entityComponentUpdate,hierarchy,&Hierarchy::UpdateComponent);
    connect(networkManager,&NetworkManager::removeEntity,hierarchy,&Hierarchy::removeEntity);
    connect(networkManager,&NetworkManager::updateScene,scenerenderer,&SceneRenderer::Render);
    connect(networkManager,
            &NetworkManager::renameEntity,
            hierarchy,
            &Hierarchy::renameEntity);

    // --- START CLIENT SIMULATION AUTOMATICALLY ---by Aman
    connect(networkManager, &NetworkManager::initSyncComplete,
            this, [this]() {

                if (!networkManager->isServer()) {
                    qDebug() << "[CLIENT] Starting simulation loop";
                    //simulation->start();
                }
            });
    connect(hierarchy, &Hierarchy::status,this,[this]{
        if(simulation->isPlay){
            simulation->pause();
            QThread::msleep(200);
        }
    });


    //Shared Memory
    //connect(simulation,&Simulation::sendMode,sharedmemory,&SharedMemory::getMode);
    // 7. Connect Simulation to SharedMemory (Cross-Thread Communication)
    // The Qt::QueuedConnection is automatic because they are on different threads
    // connect(simulation, &Simulation::sendMode,
    //         sharedmemory, &SharedMemory::handleState);

    /* -------------------------------------------------------
     * Recording Implementation Start
     * ------------------------------------------------------*/
    recording = recorder->m_recording;
    replay    = recorder->m_replay;
    //recording = new Recording(hierarchy, simulation ,recorder, this);
    // connect(simulation, &Simulation::Update,
    //         recording, &Recording::getSimulationUpdate);

    connect(simulation, &Simulation::Update, this, [=]() {
        recording->getSimulationUpdate();
        hierarchy->Anlaysis();
    });
    connect(simulation, &Simulation::HierarchyUpdate,this,[=](){
        if(recording->mode == recording->recordingModes::START){
            recording->changeInHierarchy();
        }else if(recording->mode == recording->recordingModes::PAUSE){
            recording->changeInHierarchyInPause = true;
        }
    });

    connect(recorder,&Recorder::setSQLite,this,[=](){
        recorder->m_sqlite = sqlite->getSQLite();
    });
    connect(recorder,&Recorder::fileToDB, this,[=](QString path, bool &isFileSaved){
        sqlite->dbInit(path,isFileSaved);
    });
    connect(recorder,&Recorder::loadToDB, this,[=](QString path, bool &isFileLoaded){
        sqlite->dbConnect(path,isFileLoaded);
    });


    connect(replay,&Replay::updateScene,scenerenderer,&SceneRenderer::Render);
    connect(replay,&Replay::createProfileCategories,hierarchy,&Hierarchy::addProfileCategaoryWithObject);
    connect(replay,&Replay::deleteProfileCategories,hierarchy,&Hierarchy::removeProfileCategaory);
    connect(replay,&Replay::createEntitiesCreate,hierarchy,&Hierarchy::addEntityViaLogger);
    connect(replay,&Replay::deleteEntities,hierarchy,&Hierarchy::removeEntity);
    /* -------------------------------------------------------
     * Recording Implementation End
     * ------------------------------------------------------*/

    QObject::connect(simulationThread, &QThread::started, simulation, &Simulation::init);
    connect(simulationThread, &QThread::finished, simulationThread, &QThread::deleteLater);
    simulationThread->start();


}

Runtime::~Runtime() {
    std::cout << "Runtime delete";
    Console::log("Runtime delete");
    delete sharedWrapper;
    delete recording;
    delete replay;
    delete recorder;
    // delete missionExcuter;
    // delete sharedmemory; //Shared Memory By Himanshu
    // Clean up Shared Memory Thread New
    if (sharedMemoryThread && sharedMemoryThread->isRunning()) {
        sharedMemoryThread->quit();
        sharedMemoryThread->wait();
    }
    // sharedmemory is deleted by deleteLater connection New
    delete networkManager;
    delete scenerenderer;
    delete simulation;
    delete Library;
    delete hierarchy;
    //Added by aman
    if (simulationThread && simulationThread->isRunning()) {
        simulationThread->quit();
        simulationThread->wait();
    }
    // SQLite by Himanshu
    delete sqlite;
    delete console;
}

void Runtime::handleStart() {
    if (simulation) simulation->start();
}

void Runtime::handleStop() {
    if (simulation) simulation->stop();
}

void Runtime::handleReplay() {
    if (simulation) {
        simulation->replay();
    }
}

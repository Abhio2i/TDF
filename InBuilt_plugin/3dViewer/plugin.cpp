#include "plugin.h"
#include "attitudeindicator.h"
#include <QVBoxLayout>
#include <QPushButton>
#include <QLabel>
// #include <iostream>

Plugin::Plugin() {
    // AttitudeIndicator ka object banaya
    instrumentWindow = new AttitudeIndicator();

    // Isko alag window ke taur par show kar diya
    instrumentWindow->show();
}

// plugin.cpp ke end me yeh naya destructor function add karein:

Plugin::~Plugin() {
    if (instrumentWindow) {
        instrumentWindow->close();        // Window ko screen se hatayega
        delete instrumentWindow;          // Memory se clean karein
        instrumentWindow = nullptr;
    }
}
void Plugin::entityAdded(std::string id,std::string type){
    // std::cout <<"entity add::::"<< id;
}

void Plugin::entityRemoved(std::string id){
    // std::cout <<"entity remove::::"<< id;
}

void Plugin::entitySelected(std::string id){
    // std::cout <<"entity select::::"<< id;
    selectedEntity = id;
}

void Plugin::update(float delta){
    // std::cout <<"entity update::::"<< delta;
    if(!selectedEntity.empty()){
        HierarchyInterface::geoData data = getEntityGeoDataByID(selectedEntity);
        instrumentWindow->pitch = data.pitch;
        instrumentWindow->roll = -data.roll;
        instrumentWindow->heading = data.yaw;
        instrumentWindow->altitude = data.alt;
        instrumentWindow->speed = 800;
        instrumentWindow->update();
    }
}

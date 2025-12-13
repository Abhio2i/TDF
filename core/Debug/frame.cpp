#include "frame.h"

Frame::Frame() {}

void Frame::analyze(){
    excutionTime = canvasTime+physicsTime;
}

void Frame::clean(){
     number = 0;
     excutionTime = 0;//ms
     GUITime = 0;//ms
     canvasTime = 0;//ms
     physicsTime = 0;//ms
     dynamicTime = 0;//ms
     SensorTime = 0;//ms
     RadarTime = 0;//ms
     EWTime = 0;//ms
     CSMTime = 0;//ms
     ESMTime = 0;//ms
     IFFTime = 0;//ms
     RadioTime = 0;//ms
     totalEntity = 0;
     totalSensor = 0;
     totalRadio = 0;
     csmdisplay = 0;
       totalIff = 0;
       esmdisplay = 0;
     totaldynamicModel = 0;

}

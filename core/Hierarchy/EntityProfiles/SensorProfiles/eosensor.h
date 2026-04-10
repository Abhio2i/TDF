#ifndef EOSENSOR_H
#define EOSENSOR_H

#include "eoSensorLib/eo.h"
#include "eoSensorLib/eo_ir.h"
#include <QObject>

#include <core/Hierarchy/EntityProfiles/sensor.h>
#include "core/Hierarchy/profilecategaory.h"
class EOSensor: public Sensor
{
    Q_OBJECT
public:
    Hierarchy* m_h;
    explicit EOSensor(Hierarchy* h);
    std::unordered_map<std::string, Platform*> *m_Platforms;
    std::unordered_map<std::string, Specialzone*> *m_Specialzones;

    Surrounding getSurrounding(double radius   = 10000.0,
                               double atmCoeff = 0.00001,  // %
                               double rainRate = 0.0,      // mm/h
                               double fog      = 10000.0,  // %
                               double humidity = 0.5       // %
                               );
    EO_IR* eo_ir;
    EO* eo = nullptr;
    // float frequency = 8;//ghz
    // float azimuth = 60;//deg
    // float range = 100;//km
    void scan() override;
    QJsonObject toJson() const override;
    void fromJson(const QJsonObject& obj) override;

    /*------------    Custom Debugger Start    ------------*/
private:
    /*   General purpose sting For Passing   */
    QString str;

    /*  Custom enum for Selective Debugging  */
public:
    typedef enum {
        D_NULL            = 0b10000000000000,
        D_JustPrint       = 0b01000000000000,
        D_INIT            = 0b00100000000000,
        D_Connect         = 0b00010000000000,
        D_GetPayLoad      = 0b00001000000000,
        D_SetPayLoad      = 0b00000100000000,
        D_Trajectory      = 0b00000010000000,
        D_LoadInBetween   = 0b00000001000000,
    }debugSQLite;
    Q_ENUM(debugSQLite)

private:
    /*   To Print Above String   */
    void debug(const QString &str,const debugSQLite &currentdebugType = D_JustPrint);
    /*   Variable which hold the value for
     *   Custom Debugging    */
    /*  ===> " USE ME " for debugging   <===*/
    int debugList = D_JustPrint
                    | D_INIT
        ;
    /*   To find the the debugOptions inside
     *   debugType or not "Helping Function" */
    bool dbgIsAllow(const debugSQLite &currentdebugType);

    /*------------     Custom Debugger End     ------------*/
};

#endif // EOSENSOR_H

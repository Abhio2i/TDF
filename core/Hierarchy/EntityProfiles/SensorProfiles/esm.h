#ifndef ESM_H
#define ESM_H
#include "core/Hierarchy/EntityProfiles/sensor.h"


struct ESMTarget {
    Platform *entity;
    double angle = 0.0;
    double radius = 0.0;
    quint32 timesec;
    quint32 time_frac;
    quint32 emitterid;
    char emitter_name;
    quint32 pri;
    quint16 frequency;
    quint8 frequencytype;
    quint8 pritype;
    quint32 pw;
    quint16 platformed;
};
class ESM: public Sensor
{
    Q_OBJECT
public:
    explicit ESM(Hierarchy* h);
    // float frequency = 8;//ghz
    // float azimuth = 60;//deg
    // float range = 100;//km
    QVector<ESMTarget> detect;
    void scan() override;
    QJsonObject toJson() const override;
    void fromJson(const QJsonObject& obj) override;
    QJsonObject AdditionalParameters;   //!< User-defined extension parameters

};

#endif // ESM_H

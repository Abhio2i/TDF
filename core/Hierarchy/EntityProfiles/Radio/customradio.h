#ifndef CUSTOMRADIO_H
#define CUSTOMRADIO_H

#include "core/Hierarchy/EntityProfiles/radio.h"
#include "core/Plugins/Interfaces/customRadioInterface.h"
class CustomRadio:public Radio
{
    Q_OBJECT
public:
    explicit CustomRadio(Hierarchy* h);
    ~CustomRadio();
    QString type = "";
    CustomRadioInterface*  radio = nullptr;


    QJsonObject toJson()                     const override;
    void        fromJson(const QJsonObject&)       override;

    QJsonObject AdditionalParameters;   //!< User-defined extension parameters

};

#endif // CUSTOMRADIO_H

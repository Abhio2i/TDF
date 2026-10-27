#ifndef CUSTOMIFF_H
#define CUSTOMIFF_H

#include "core/Hierarchy/EntityProfiles/iff.h"
#include "core/Plugins/Interfaces/customIFFInterface.h"
class CustomIFF:public IFF
{
    Q_OBJECT
public:
    explicit CustomIFF(Hierarchy* h);
    ~CustomIFF();
    QString type = "";
    CustomIFFInterface*  iff = nullptr;


    QJsonObject toJson()                     const override;
    void        fromJson(const QJsonObject&)       override;

    QJsonObject AdditionalParameters;   //!< User-defined extension parameters

};

#endif // CUSTOMIFF_H

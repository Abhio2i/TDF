#ifndef PLUGIN_H
#define PLUGIN_H

#include "attitudeindicator.h"
#include "plugin_global.h"
#include "qobject.h"
#include "HierarchyInterface.h"
class PLUGIN_EXPORT Plugin: public QObject, public HierarchyInterface {
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "com.linux.HierarchyInterface/1.0")
    Q_INTERFACES(HierarchyInterface)
public:
    Plugin();
    ~Plugin() override;
    void entityAdded(std::string id,std::string type) override;
    void entityRemoved(std::string id) override;
    void entitySelected(std::string id) override;

    void update(float delta) override;
private:
    AttitudeIndicator *instrumentWindow;
    std::string selectedEntity;
    // Parameter liye aur result return kar diya

};

#endif // PLUGIN_H

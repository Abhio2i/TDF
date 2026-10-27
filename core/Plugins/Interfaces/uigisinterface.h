#ifndef UIGISINTERFACE_H
#define UIGISINTERFACE_H

#include <QtPlugin>
#include <QString>
#include <QJsonObject>
#include <functional>
struct UiActionContext;
class QWidget;
class IShapeType;
class ILayerFormat;

class UIGISInterface

{
public:
    virtual ~UIGISInterface() = default;

    virtual void initializeGIS() = 0;
    virtual void unloadGIS() = 0;

    // ===== existing: toolbar / context / shape / format / tile =====
    using RegisterGisActionCallback = std::function<void(
        const QString& id, const QString& label, const QString& iconPath,
        const QString& category, bool checkable, int priority,
  std::function<void(const UiActionContext&)> onTriggered)>;
    RegisterGisActionCallback registerGisAction = nullptr;
    void setRegisterGisAction(RegisterGisActionCallback cb) { registerGisAction = cb; }

    using RegisterGisContextCallback = std::function<void(
        const QString& id, const QString& label, const QString& contextType,
        const QString& group, int priority,
        std::function<void(const QString&, const QString&)> onTriggered)>;
    RegisterGisContextCallback registerGisContext = nullptr;
    void setRegisterGisContext(RegisterGisContextCallback cb) { registerGisContext = cb; }

    using RegisterShapeTypeCallback = std::function<void(IShapeType*)>;
    RegisterShapeTypeCallback registerShapeType = nullptr;
    void setRegisterShapeType(RegisterShapeTypeCallback cb) { registerShapeType = cb; }

    using RegisterLayerFormatCallback = std::function<void(ILayerFormat*)>;
    RegisterLayerFormatCallback registerLayerFormat = nullptr;
    void setRegisterLayerFormat(RegisterLayerFormatCallback cb) { registerLayerFormat = cb; }

    using RegisterTileSourceCallback = std::function<void(
        const QString& id, const QString& folderName, const QString& displayName,
        const QString& iconPath, int priority)>;
    RegisterTileSourceCallback registerTileSource = nullptr;
    void setRegisterTileSource(RegisterTileSourceCallback cb) { registerTileSource = cb; }

    //  host helpers
    using SetCenterFn = std::function<void(double lat, double lon)>;
    using SetZoomFn   = std::function<void(int level)>;
    using AddShapeFn  = std::function<QString(const QJsonObject& data)>;
    using StartDrawingFn  = std::function<void(const QString& shapeType)>;
    SetCenterFn setMapCenter = nullptr;
    SetZoomFn   setMapZoom   = nullptr;
    AddShapeFn  addShape     = nullptr;
    StartDrawingFn  startDrawingShape = nullptr;
};

#define UIGISInterface_iid "com.o2i.UIGISInterface/1.0"
Q_DECLARE_INTERFACE(UIGISInterface, UIGISInterface_iid)

#endif

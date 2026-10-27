#ifndef UITOOLBARINTERFACE_H
#define UITOOLBARINTERFACE_H

#include <QtPlugin>
#include <QAction>
#include <QString>
#include <QMenu>
#include <functional>

class UIToolbarInterface
{
public:
    virtual ~UIToolbarInterface() = default;
    virtual void initializeUI() = 0;
    virtual void unloadUI() = 0;

    // DesignToolBar
    using RegisterDesignActionCallback = std::function<void(QAction* action, const QString& pluginId)>;
    RegisterDesignActionCallback registerDesignAction = nullptr;
    void registerDesignActionCallback(RegisterDesignActionCallback cb) {
        registerDesignAction = cb;
    }

    // RuntimeToolBar
    using RegisterRuntimeActionCallback = std::function<void(QAction* action, const QString& pluginId)>;
    RegisterRuntimeActionCallback registerRuntimeAction = nullptr;
    void registerRuntimeActionCallback(RegisterRuntimeActionCallback cb) {
        registerRuntimeAction = cb;
    }
    // ========== NetworkToolBar ==========
    using RegisterNetworkActionCallback = std::function<void(QAction* action, const QString& pluginId)>;
    RegisterNetworkActionCallback registerNetworkAction = nullptr;
    void registerNetworkActionCallback(RegisterNetworkActionCallback cb) {
        registerNetworkAction = cb;
    }
    // MenuBar (menuName = "File", "Edit", "Tools" etc.)
    using RegisterMenuActionCallback = std::function<void(const QString& menuName, QAction* action, const QString& pluginId)>;
    RegisterMenuActionCallback registerMenuAction = nullptr;
    void registerMenuActionCallback(RegisterMenuActionCallback cb) {
        registerMenuAction = cb;
    }
};

#define UIToolbarInterface_iid "com.o2i.UIToolbarInterface/1.0"
Q_DECLARE_INTERFACE(UIToolbarInterface, UIToolbarInterface_iid)

#endif

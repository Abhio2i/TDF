
#ifndef SCRIPTENGINE_H
#define SCRIPTENGINE_H

#include "GUI/Hierarchytree/hierarchytree.h"
#include "core/Hierarchy/hierarchy.h"
#include "core/Render/scenerenderer.h"
#include <QObject>
#include <angelscript.h>

class MyObj {
public:
    float x, y;
    MyObj() : x(0), y(0) {};
    void moveBy(float dx, float dy)
    {
        this->x += dx;
        this->y += dy;
    }

};

class ScriptEngine : public QObject
{
    Q_OBJECT

public:
    ScriptEngine();
    ~ScriptEngine();

    void setHierarchy(Hierarchy *hier, HierarchyTree *tr, SceneRenderer *rend);
    bool loadAndCompileScript(QString scriptContent);
    void run();

public slots:
    void ScriptSleep(int milliseconds);
    ProfileCategaory* addProfiles(const std::string &name);
    Platform *addEntity(const std::string &Id, const std::string &name, bool &profile);
    Folder* addFolder(const std::string &Id, const std::string &name, bool &profile);
    void addComponent(const std::string &Id,const std::string &name);
    void renderscene();

public:
    Hierarchy *hierarchy;
    HierarchyTree *tree;
    SceneRenderer *render;
    MyObj* e = nullptr;

private:
    asIScriptEngine* engine = nullptr;
    asIScriptModule* mod = nullptr;
    asIScriptFunction* func = nullptr;
    asIScriptContext* ctx = nullptr;
};

#endif // SCRIPTENGINE_H

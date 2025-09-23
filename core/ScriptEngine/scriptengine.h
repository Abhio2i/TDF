
#ifndef SCRIPTENGINE_H
#define SCRIPTENGINE_H

#include "GUI/Hierarchytree/hierarchytree.h"
#include "core/Hierarchy/hierarchy.h"
#include "core/Render/scenerenderer.h"
#include "core/Hierarchy/Struct/parameter.h"
#include <QObject>
#include <angelscript.h>
#include <angelscript/add_on/scriptarray/scriptarray.h>

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
    // Add Parameter to an existing entity by ID
    // void addParameterToEntity(const std::string& entityId,
    //                           const std::string& paramName,
    //                           int paramType,
    //                           const std::string& paramValue);
    // === Renaming functions ===
    void renameEntity(const std::string &id, const std::string &newName);
    // Rename a profile by ID
    void renameProfile(const std::string& profileID, const std::string& newName);
    // Wrapper for AngelScript
    void renameFolder(const std::string& folderID, const std::string& newName);
    // ------------------ Remove Profile Wrapper ------------------
    // Usage in AS: removeProfile("profileId123");
    void removeProfile(const std::string& profileID);
    // Remove Folder wrapper for AngelScript
    void removeFolder(const std::string& parentId, const std::string& folderID);
    // Remove Entity wrapper for AngelScript
    void removeEntity(const std::string& parentId, const std::string& entityID);
    // === Finder functions ===
    Entity* getEntityById(const std::string &id);
    CScriptArray* findEntitiesByType(int type);
    CScriptArray* getAllEntities();
    CScriptArray* getAllEntityIds();

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
    asITypeInfo* arrayEntityType = nullptr; // ⚡ store type info once
};

#endif // SCRIPTENGINE_H

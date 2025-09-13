#include "scriptengine.h"
#include "qtimer.h"
#include <angelscript.h>
#include <angelscript/add_on/scriptstdstring/scriptstdstring.h>
#include <QMetaObject>
#include <QDebug>
#include <string>
#include <QObject>


#include <cmath>
#include <cstdlib>

    // Random number generator
static float Math_Random()
{
    return static_cast<float>(rand()) / static_cast<float>(RAND_MAX);
}

// Trigonometric functions
static float Math_Sin(float angle) { return sin(angle); }
static float Math_Cos(float angle) { return cos(angle); }
static float Math_Tan(float angle) { return tan(angle); }
static float Math_ASin(float val) { return asin(val); }
static float Math_ACos(float val) { return acos(val); }
static float Math_ATan(float val) { return atan(val); }
static float Math_ATan2(float y, float x) { return atan2(y, x); }

// Exponential and power functions
static float Math_Pow(float base, float exp) { return pow(base, exp); }
static float Math_Sqrt(float val) { return sqrt(val); }
static float Math_Exp(float val) { return exp(val); }
static float Math_Log(float val) { return log(val); }
static float Math_Log10(float val) { return log10(val); }

// Rounding and absolute value functions
static float Math_Abs(float val) { return abs(val); }
static float Math_Floor(float val) { return floor(val); }
static float Math_Ceil(float val) { return ceil(val); }
static float Math_Round(float val) { return round(val); }

// Constant values
static const float Math_PI = 3.14159265359f;

void MessageCallback(const asSMessageInfo *msg, void *param)
{
    const char *type = "ERR";
    if (msg->type == asMSGTYPE_WARNING){
        type = "WARN";
        qWarning() << msg->section << "(" << msg->row << "," << msg->col << "):" << type << ":" << msg->message;
    }else if (msg->type == asMSGTYPE_INFORMATION){
        type = "INFO";
        qDebug() << msg->section << "(" << msg->row << "," << msg->col << "):" << type << ":" << msg->message;
    }else if (msg->type == asMSGTYPE_ERROR){
        type = "ERROR";
        qDebug() << msg->section << "(" << msg->row << "," << msg->col << "):" << type << ":" << msg->message;
    }

}

void Print(const std::string &msg)
{
    qDebug() << "[AngelScript]:" << QString::fromStdString(msg);
}



ProfileCategaory* ScriptEngine::addProfiles(const std::string &name)
{
    qDebug() << "Hierarchy object address from script engine OUTR:" << this;//hierarchy;
    if (hierarchy){
        ProfileCategaory* Profile = hierarchy->addProfileCategaory(QString::fromStdString(name));
        Profile->setProfileType(Constants::EntityType::Platform);
        return Profile;
    }
    return nullptr;
}

Folder* ScriptEngine::addFolder(const std::string &Id,const std::string &name,bool &profile)
{
    qDebug() << "Hierarchy object address from script engine OUTR:" << this;//hierarchy;
    if (hierarchy){
        return hierarchy->addFolder(QString::fromStdString(Id),QString::fromStdString(name),profile);
    }
    return nullptr;
}

Platform* ScriptEngine::addEntity(const std::string &Id,const std::string &name, bool &profile)
{
    qDebug() << "Hierarchy object address from script engine OUTR:" << this;//hierarchy;
    if (hierarchy){
        Platform* entity = static_cast<Platform*>(hierarchy->addEntity(QString::fromStdString(Id),QString::fromStdString(name),profile));
        // hierarchy->addComponent(QString::fromStdString(entity->ID),"transform");
        hierarchy->addComponent(QString::fromStdString(entity->ID),"dynamicModel");
        hierarchy->addComponent(QString::fromStdString(entity->ID),"meshRenderer2d");
        return entity;
    }
    return nullptr;
}

void ScriptEngine::addComponent(const std::string &Id,const std::string &name)
{
    qDebug() << "Hierarchy object address from script engine OUTR:" << this;//hierarchy;
    if (hierarchy){
        hierarchy->addComponent(QString::fromStdString(Id),QString::fromStdString(name));
    }
}

// scriptengine.cpp
void ScriptEngine::ScriptSleep(int milliseconds)
{
    ctx->Suspend();
    QTimer::singleShot(milliseconds, [this]() {
        this->ctx->Execute();
    });
}

void ScriptEngine::renderscene(){
    if (render){
        emit render->Render(0.01);
    }
}

ScriptEngine::ScriptEngine()
{
    // AngelScript setup
    engine = asCreateScriptEngine();
    engine->SetMessageCallback(asFUNCTION(MessageCallback), 0, asCALL_CDECL);

    RegisterStdString(engine);
    engine->RegisterGlobalFunction("float Random()", asFUNCTION(Math_Random), asCALL_CDECL);

    // Trigonometric functions
    engine->RegisterGlobalFunction("float Sin(float)", asFUNCTION(Math_Sin), asCALL_CDECL);
    engine->RegisterGlobalFunction("float Cos(float)", asFUNCTION(Math_Cos), asCALL_CDECL);
    engine->RegisterGlobalFunction("float Tan(float)", asFUNCTION(Math_Tan), asCALL_CDECL);
    engine->RegisterGlobalFunction("float ASin(float)", asFUNCTION(Math_ASin), asCALL_CDECL);
    engine->RegisterGlobalFunction("float ACos(float)", asFUNCTION(Math_ACos), asCALL_CDECL);
    engine->RegisterGlobalFunction("float ATan(float)", asFUNCTION(Math_ATan), asCALL_CDECL);
    engine->RegisterGlobalFunction("float ATan2(float, float)", asFUNCTION(Math_ATan2), asCALL_CDECL);

    // Exponential and power functions
    engine->RegisterGlobalFunction("float Pow(float, float)", asFUNCTION(Math_Pow), asCALL_CDECL);
    engine->RegisterGlobalFunction("float Sqrt(float)", asFUNCTION(Math_Sqrt), asCALL_CDECL);
    engine->RegisterGlobalFunction("float Exp(float)", asFUNCTION(Math_Exp), asCALL_CDECL);
    engine->RegisterGlobalFunction("float Log(float)", asFUNCTION(Math_Log), asCALL_CDECL);
    engine->RegisterGlobalFunction("float Log10(float)", asFUNCTION(Math_Log10), asCALL_CDECL);

    // Rounding and absolute value functions
    engine->RegisterGlobalFunction("float Abs(float)", asFUNCTION(Math_Abs), asCALL_CDECL);
    engine->RegisterGlobalFunction("float Floor(float)", asFUNCTION(Math_Floor), asCALL_CDECL);
    engine->RegisterGlobalFunction("float Ceil(float)", asFUNCTION(Math_Ceil), asCALL_CDECL);
    engine->RegisterGlobalFunction("float Round(float)", asFUNCTION(Math_Round), asCALL_CDECL);

    // Constant
    engine->RegisterGlobalProperty("const float PI", (void*)&Math_PI);

    // Register print + profile functions
    engine->RegisterGlobalFunction("void Print(const string &in)", asFUNCTION(Print), asCALL_CDECL);
    // engine->RegisterGlobalFunction("void sleep(int milliseconds)", asFUNCTION(ScriptSleep), asCALL_CDECL);

    int s;
    s = engine->RegisterObjectType("MyObj", 0, asOBJ_REF | asOBJ_NOCOUNT); Q_ASSERT(s >= 0);
    s = engine->RegisterObjectProperty("MyObj", "float x", asOFFSET(MyObj, x)); Q_ASSERT(s >= 0);
    s = engine->RegisterObjectProperty("MyObj", "float y", asOFFSET(MyObj, y)); Q_ASSERT(s >= 0);
    s = engine->RegisterObjectMethod("MyObj", "void moveBy(float dx, float dy)", asMETHOD(MyObj, moveBy), asCALL_THISCALL); Q_ASSERT(s >= 0);

    s = engine->RegisterObjectType("Vector", 0, asOBJ_REF | asOBJ_NOCOUNT); Q_ASSERT(s >= 0);
    s = engine->RegisterObjectProperty("Vector", "float x", asOFFSET(Vector, x)); Q_ASSERT(s >= 0);
    s = engine->RegisterObjectProperty("Vector", "float y", asOFFSET(Vector, y)); Q_ASSERT(s >= 0);
    s = engine->RegisterObjectProperty("Vector", "float z", asOFFSET(Vector, z)); Q_ASSERT(s >= 0);

    s = engine->RegisterObjectType("Transform", 0, asOBJ_REF | asOBJ_NOCOUNT); Q_ASSERT(s >= 0);
    s = engine->RegisterObjectProperty("Transform", "Vector@ position", asOFFSET(Transform, position)); Q_ASSERT(s >= 0);
    s = engine->RegisterObjectProperty("Transform", "Vector@ rotation", asOFFSET(Transform, rotation)); Q_ASSERT(s >= 0);
    s = engine->RegisterObjectProperty("Transform", "Vector@ size", asOFFSET(Transform, size)); Q_ASSERT(s >= 0);

    s = engine->RegisterObjectType("ProfileCategaory", 0, asOBJ_REF | asOBJ_NOCOUNT); Q_ASSERT(s >= 0);
    s = engine->RegisterObjectProperty("ProfileCategaory", "string id", asOFFSET(ProfileCategaory, ID)); Q_ASSERT(s >= 0);
    s = engine->RegisterObjectProperty("ProfileCategaory", "string name", asOFFSET(ProfileCategaory, Name)); Q_ASSERT(s >= 0);

    s = engine->RegisterObjectType("Folder", 0, asOBJ_REF | asOBJ_NOCOUNT); Q_ASSERT(s >= 0);
    s = engine->RegisterObjectProperty("Folder", "string id", asOFFSET(Folder, ID)); Q_ASSERT(s >= 0);
    s = engine->RegisterObjectProperty("Folder", "string name", asOFFSET(Folder, Name)); Q_ASSERT(s >= 0);

    s = engine->RegisterObjectType("Platform", 0, asOBJ_REF | asOBJ_NOCOUNT); Q_ASSERT(s >= 0);
    s = engine->RegisterObjectProperty("Platform", "string id", asOFFSET(Entity, ID)); Q_ASSERT(s >= 0);
    s = engine->RegisterObjectProperty("Platform", "string name", asOFFSET(Entity, Name)); Q_ASSERT(s >= 0);
    s = engine->RegisterObjectProperty("Platform", "Transform@ transform", asOFFSET(Platform, transform)); Q_ASSERT(s >= 0);


    s = engine->RegisterObjectType("ScriptEngine", 0, asOBJ_REF | asOBJ_NOCOUNT); Q_ASSERT(s >= 0);
    s = engine->RegisterObjectMethod("ScriptEngine", "void render()", asMETHOD(ScriptEngine, renderscene), asCALL_THISCALL); Q_ASSERT(s >= 0);
    s = engine->RegisterObjectMethod("ScriptEngine", "void sleep(int milliseconds)", asMETHOD(ScriptEngine, ScriptSleep), asCALL_THISCALL); Q_ASSERT(s >= 0);
    s = engine->RegisterObjectMethod("ScriptEngine", "ProfileCategaory@ NewProfile(const string &in)", asMETHOD(ScriptEngine, addProfiles), asCALL_THISCALL); Q_ASSERT(s >= 0);
    s = engine->RegisterObjectMethod("ScriptEngine", "Platform@ addEntity(const string &in,const string &in, const bool &in)", asMETHOD(ScriptEngine, addEntity), asCALL_THISCALL); Q_ASSERT(s >= 0);
    s = engine->RegisterObjectMethod("ScriptEngine", "Folder@ addFolder(const string &in,const string &in, const bool &in)", asMETHOD(ScriptEngine, addFolder), asCALL_THISCALL); Q_ASSERT(s >= 0);
    s = engine->RegisterObjectMethod("ScriptEngine", "void addComponent(const string &in,const string &in)", asMETHOD(ScriptEngine, addComponent), asCALL_THISCALL); Q_ASSERT(s >= 0);

    e = new MyObj();
    e->x = 10;
    e->y = 20;

    // Sample script
    // const char *script =
    //     "void main() { \n"
    //     "   Print('Hello from AngelScript + Qt!'); \n"
    //     "   //NewProfile('TestProfile'); \n"
    //     "}";



    mod = engine->GetModule("MyModule", asGM_ALWAYS_CREATE);
    // mod->AddScriptSection("script", script);

    // int r = mod->Build();
    // if (r < 0) {
    //     qDebug() << "Failed to compile script";
    //     return;
    // }

    // func = mod->GetFunctionByDecl("void main()");
    // if (!func) {
    //     qDebug() << "Function not found";
    //     return;
    // }

    // ctx = engine->CreateContext();
    // const char* script =
    //     "void on_update(MyObj@ e) { \n"
    //     "  Print('Before: x=' + e.x + ', y=' + e.y); \n"
    //     "  e.x += 5; \n"
    //     "  e.y += 10; \n"
    //     "  e.moveBy(100,100); \n"
    //     "  Print('After: x=' + e.x + ', y=' + e.y); \n"
    //     "}";
    // const char* script =
    //     "void on_update(ScriptEngine@ e) { \n"
    //     "  Print('Before'); \n"
    //     "  string d = e.NewProfile('hello'); \n"
    //     "  e.addEntity(d, 'entity',true); \n"
    //     "  Print('After'); \n"
    //     "}";
    // mod = engine->GetModule("MyModule", asGM_ALWAYS_CREATE);
    // mod->AddScriptSection("script", script);
    // if (mod->Build() < 0) {
    //     qDebug() << "Script Build Failed!";
    //     return;
    // } else {
    //     qDebug() << "Script Compiled.";
    // }

    ctx = engine->CreateContext();
}

ScriptEngine::~ScriptEngine()
{
}

void ScriptEngine::setHierarchy(Hierarchy *hiers, HierarchyTree *tr, SceneRenderer *rend)
{
    hierarchy = hiers;
    tree = tr;
    render = rend;
}

bool ScriptEngine::loadAndCompileScript(QString scriptContent)
{
    engine->DiscardModule("MyModule");
    mod = engine->GetModule("MyModule", asGM_ALWAYS_CREATE);
    mod->AddScriptSection("script", scriptContent.toUtf8().data());

    int r = mod->Build();
    if (r < 0) {
        qDebug() << "Failed to compile script!";
        return false;
    }

    func = mod->GetFunctionByName("main");
    if (!func) {
        qDebug() << "Function main() not found!";
        return false;
    }
    ctx->Prepare(func);
    ctx->SetArgObject(0, this);
    run();

    // //addProfiles("hrgff");

    // asIScriptFunction* func = mod->GetFunctionByName("on_update");
    // if (!func) {
    //     qDebug() << "on_update() function not found!";
    //     return false;
    // }

    // ctx->Prepare(func);
    // ctx->SetArgObject(0, this);
    // int r = ctx->Execute();
    // if (r != asEXECUTION_FINISHED) {
    //     qDebug() << "Script execution failed!";
    // } else {
    //     qDebug() << "Executed. Entity now: x =" << e->x << ", y =" << e->y;
    // }
    // //ctx->Release();
    return true;
}

void ScriptEngine::run()
{
    if (!ctx || !func)
        return;

    //ctx->Prepare(func);
    ctx->Execute();
}


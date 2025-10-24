#ifndef SESSIONMANAGER_H
#define SESSIONMANAGER_H
#include <QObject>
#include <variant>

class SessionManager: public QObject
{
    Q_OBJECT
public:
    SessionManager();
    std::unordered_map<std::string,  std::variant<int, float, double, std::string, bool, char>> *prefs;
    std::unordered_map<std::string,  std::variant<int, float, double, std::string, bool, char>> *tempprefs;

    void cleanPrefs();
    void getPrefs();
    void setPrefs();
    void isPrefs();
    void getPrefType();

    void toJson();
    void fromJson();
};

#endif // SESSIONMANAGER_H

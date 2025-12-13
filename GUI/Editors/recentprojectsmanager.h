
#ifndef RECENTPROJECTSMANAGER_H
#define RECENTPROJECTSMANAGER_H
#include <QObject>
#include <QString>
#include <QStringList>
#include <QSettings>

class RecentProjectsManager : public QObject
{
    Q_OBJECT

public:
    enum EditorType {
        ScenarioEditor,
        RuntimeEditor,
        DatabaseEditor
    };

    static RecentProjectsManager* instance();

    void addToRecentProjects(const QString &filePath, EditorType editorType);
    QStringList getRecentProjects(EditorType editorType) const;
    void clearRecentProjects(EditorType editorType);
    void showRecentProjectsMenu(QWidget *parent, EditorType editorType);

signals:
    void projectSelected(const QString& filePath, EditorType editorType);

private:
    RecentProjectsManager(QObject *parent = nullptr);
    static RecentProjectsManager* m_instance;

    QString getSettingsKey(EditorType editorType) const;
};
 #endif

#ifndef SETTINGSMANAGER_H
#define SETTINGSMANAGER_H

#include <QObject>
#include <QSettings>
#include <QString>
#include <QDateTime>  // ADD THIS
#include <QStringList>

class SettingsManager : public QObject
{
    Q_OBJECT

public:
    static SettingsManager& instance();

    // Recent files management
    void addRecentFile(const QString& filePath);
    QStringList getRecentFiles() const;
    void clearRecentFiles();

    // Last session management
    void saveLastSession(const QString& filePath, const QString& editorType);
    QString getLastSessionFile() const;
    QString getLastSessionEditor() const;
    bool hasLastSession() const;

    // Window state and geometry
    void saveWindowState(const QByteArray& state);
    QByteArray getWindowState() const;

    void saveWindowGeometry(const QByteArray& geometry);
    QByteArray getWindowGeometry() const;

private:
    SettingsManager();
    ~SettingsManager() = default;

    QSettings m_settings;
};

#endif // SETTINGSMANAGER_H

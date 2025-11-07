#include "settingsmanager.h"
#include <QDateTime>  // ADD THIS

SettingsManager& SettingsManager::instance()
{
    static SettingsManager instance;
    return instance;
}

SettingsManager::SettingsManager()
    : m_settings("YourCompany", "YourApp")
{
}

void SettingsManager::addRecentFile(const QString& filePath)
{
    if (filePath.isEmpty()) return;

    QStringList recentFiles = getRecentFiles();

    // Remove if already exists
    recentFiles.removeAll(filePath);

    // Add to beginning
    recentFiles.prepend(filePath);

    // Keep only last 10 files
    while (recentFiles.size() > 10) {
        recentFiles.removeLast();
    }

    m_settings.setValue("recent/files", recentFiles);
}

QStringList SettingsManager::getRecentFiles() const
{
    return m_settings.value("recent/files").toStringList();
}

void SettingsManager::clearRecentFiles()
{
    m_settings.remove("recent/files");
}

void SettingsManager::saveLastSession(const QString& filePath, const QString& editorType)
{
    m_settings.setValue("lastSession/filePath", filePath);
    m_settings.setValue("lastSession/editorType", editorType);
    m_settings.setValue("lastSession/timestamp", QDateTime::currentDateTime());
}

QString SettingsManager::getLastSessionFile() const
{
    return m_settings.value("lastSession/filePath").toString();
}

QString SettingsManager::getLastSessionEditor() const
{
    return m_settings.value("lastSession/editorType").toString();
}

bool SettingsManager::hasLastSession() const
{
    return m_settings.contains("lastSession/filePath") &&
           !m_settings.value("lastSession/filePath").toString().isEmpty();
}

void SettingsManager::saveWindowState(const QByteArray& state)
{
    m_settings.setValue("window/state", state);
}

QByteArray SettingsManager::getWindowState() const
{
    return m_settings.value("window/state").toByteArray();
}

void SettingsManager::saveWindowGeometry(const QByteArray& geometry)
{
    m_settings.setValue("window/geometry", geometry);
}

QByteArray SettingsManager::getWindowGeometry() const
{
    return m_settings.value("window/geometry").toByteArray();
}

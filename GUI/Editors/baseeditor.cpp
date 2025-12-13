#include "baseeditor.h"
#include "GUI/Hierarchytree/hierarchyconnector.h"
#include <QMenu>
#include <QFileInfo>
#include <QMessageBox>

void BaseEditor::onRecentProjectTriggered()
{
    setupRecentProjectsMenu();
}

void BaseEditor::setupRecentProjectsMenu()
{
    QStringList recentProjects = HierarchyConnector::instance()->getRecentProjects();

    // Filter existing files
    QStringList existingProjects;
    for (const QString& projectPath : recentProjects) {
        if (QFile::exists(projectPath)) {
            existingProjects << projectPath;
        }
    }

    // Update config with only existing projects
    if (existingProjects.size() < recentProjects.size()) {
        for (const QString& projectPath : existingProjects) {
            HierarchyConnector::instance()->addToRecentProjects(projectPath);
        }
    }

    if (existingProjects.isEmpty()) {
        QMessageBox::information(this, "Recent Projects",
                                 "📂 No recent projects found!\n\n"
                                 "Open and save a project to see it here.");
        return;
    }

    // Create menu
    QMenu recentMenu(this);
    recentMenu.setTitle("Recent Projects");

    // Add projects
    for (const QString& projectPath : existingProjects) {
        QFileInfo fileInfo(projectPath);
        QString displayText = QString("📄 %1\n    📍 %2")
                                  .arg(fileInfo.fileName())
                                  .arg(fileInfo.path());

        QAction* projectAction = recentMenu.addAction(displayText);
        projectAction->setData(projectPath);
    }

    recentMenu.addSeparator();
    recentMenu.addAction("🗑️ Clear All Recent Projects",
                         this, &BaseEditor::clearRecentProjects);

    // Show menu
    QPoint menuPos = mapToGlobal(QPoint(0, menuBar->height()));
    QAction* selectedAction = recentMenu.exec(menuPos);

    if (selectedAction && selectedAction->data().isValid()) {
        QString filePath = selectedAction->data().toString();
        loadRecentProject(filePath);
    }
}

void BaseEditor::loadRecentProject(const QString& filePath)
{
    if (!QFile::exists(filePath)) {
        QMessageBox::warning(this, "File Not Found",
                             QString("Project file not found:\n\n%1").arg(filePath));

        HierarchyConnector::instance()->getScenarioConfig().removeRecentProject(filePath);
        HierarchyConnector::instance()->saveScenarioConfig();
        return;
    }

    // Load the project
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        QMessageBox::warning(this, "Error",
                             QString("Failed to open file:\n\n%1").arg(file.errorString()));
        return;
    }

    // Parse JSON
    QByteArray data = file.readAll();
    file.close();

    QJsonParseError err;
    QJsonDocument doc = QJsonDocument::fromJson(data, &err);
    if (err.error != QJsonParseError::NoError || !doc.isObject()) {
        QMessageBox::warning(this, "Error",
                             QString("Invalid JSON file:\n\n%1").arg(err.errorString()));
        return;
    }

    QJsonObject obj = doc.object();
    if (!obj.contains("hierarchy")) {
        QMessageBox::warning(this, "Error",
                             "Invalid project file (missing 'hierarchy')");
        return;
    }

    // Load hierarchy
    QJsonObject hier = obj["hierarchy"].toObject();
    hierarchy->fromJson(hier);

    // Update scenario config
    HierarchyConnector::instance()->addToRecentProjects(filePath);
    lastSavedFilePath = filePath;
    clearUnsavedChanges();

    // Update UI
    updateUIAfterLoad();
    updateStatusBar("Loaded: " + QFileInfo(filePath).fileName());

    if (console) {
        console->log("Recent project loaded: " + filePath.toStdString());
    }
}

void BaseEditor::clearRecentProjects()
{
    HierarchyConnector::instance()->clearRecentProjects();
    updateStatusBar("Recent projects list cleared");

    if (console) {
        console->log("Recent projects list cleared");
    }
}

void BaseEditor::showProfileInfo()
{
    ProfileInfoDialog::showProfileInfo(this);
}

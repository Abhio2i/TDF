/* =============================================================================
 * FILE:         textscriptwidget.h
 * MODULE:       Text Script Management Widgets
 * PROJECT:      Indigenous Scenario and Sensor Simulation Toolkit (ISSST)
 * ORGANISATION: Oxygen 2 Innovation (O2I).
 * STANDARD:     RTCA DO-178C / ED-12C, DAL B
 * COVERAGE:     Branch / Decision Coverage required (100% true/false paths)
 *
 * DESCRIPTION:  Declares the TextScriptItemWidget and TextScriptWidget classes
 *               which provide a UI for managing and displaying AngelScript files.
 *               The item widget represents a single script with play/pause controls.
 *               The main widget displays a list of scripts, supports context menu
 *               operations (rename, remove, edit), and emits signals to run/pause
 *               scripts, rename/remove script files, and open the script editor.
 *
 * REQUIREMENTS: REQ-SCRIPTWIDGET-010  Script list widget
 *               REQ-SCRIPTWIDGET-011  Script item widget with play/pause buttons
 *               REQ-SCRIPTWIDGET-012  Load script files from directory
 *               REQ-SCRIPTWIDGET-013  Context menu: rename, remove, edit script
 *               REQ-SCRIPTWIDGET-014  Add new script button
 *               REQ-SCRIPTWIDGET-015  Signals for running, pausing, renaming,
 *                                     removing scripts
 *
 * AUTHOR:       Arti Rajpoot
 * REVIEWED BY:  [Reviewer Name], [Review Date] — SPR-SCRIPTWIDGET-001
 *
 *
 * COPYRIGHT:    Oxygen 2 Innovation (O2I). All rights reserved.
 *               Restricted circulation — defence simulation use only.
 * =============================================================================
 */

#ifndef TEXTSCRIPTWIDGET_H
#define TEXTSCRIPTWIDGET_H

#include <QWidget>                                // For widget base class
#include <QListWidget>                            // For list widget
#include <QDir>                                   // For directory handling
#include <QMenu>                                  // For menu widget
#include <QAction>                                // For action items
#include <QPushButton>                            // For push button widget
#include <QHBoxLayout>                            // For horizontal layout
#include <QVBoxLayout>                            // For vertical layout
#include <QLineEdit>

// %%% TextScriptItemWidget Class %%%
/* Widget for individual script item */
class TextScriptItemWidget : public QWidget {
    Q_OBJECT

public:
    // Initialize script item widget
    explicit TextScriptItemWidget(const QString &fileName, const QString &filePath, QWidget *parent = nullptr);
    // Play button
    QPushButton *playButton;
    // Pause button
    QPushButton *pauseButton;
    // Set active button state
    void setActiveButton(const QString &state);


signals:
    // Signal play button click
    void playClicked(const QString &filePath);
    // Signal pause button click
    void pauseClicked(const QString &filePath);
};

// %%% TextScriptWidget Class %%%
/* Widget for managing script files */
class TextScriptWidget : public QWidget {
    Q_OBJECT

public:
    // Initialize script widget
    explicit TextScriptWidget(QWidget *parent = nullptr);
     // static void runUnitTestsOnce();

signals:
    // Signal to run script code
    void runScriptstring(QString code);
    // Signal to run script file
    void runScript(const QString &filePath);
    // Signal to pause script
    void pauseScript(const QString &filePath);
    // Signal to rename script
    void renameScript(const QString &filePath, const QString &newName);
    // Signal to remove script
    void removeScript(const QString &filePath);

private slots:
    // Handle context menu
    void handleCustomContextMenu(const QPoint &pos);
    // Handle rename action
    void handleRenameAction();
    // Handle remove action
    void handleRemoveAction();
    // Handle edit action
    void handleEditAction();
    // Handle play button click
    void handlePlayClicked(const QString &filePath);
    // Handle pause button click
    void handlePauseClicked(const QString &filePath);
    // Handle add script button click
    void handleAddScriptButtonClicked();

signals:
    void runScriptFile(const QString& filePath);
    void pauseScriptFile(const QString& filePath);

private:
    // %%% Utility Methods %%%
    // Load script files from directory
    void loadScriptFiles(const QString &directoryPath);
    // Update status icon
    void updateStatusIcon(QListWidgetItem *item, const QString &status);

    // %%% UI Components %%%
    // File list widget
    QListWidget *fileListWidget;
    // Map of file statuses
    QMap<QString, QString> fileStatus;
    // Map of active button states
    QMap<QString, QString> activeButtonState;
    // Add script button
    QPushButton *addScriptButton;
    void filterScripts(const QString &text);
    void addFileToList(const QFileInfo &fileInfo);
    QLineEdit *searchBar;
    QList<QFileInfo> allFiles;
};

#endif // TEXTSCRIPTWIDGET_H

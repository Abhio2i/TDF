/* ========================================================================= */
/* File: textscriptwidget.h                                                 */
/* Purpose: Defines widgets for managing and displaying script files         */
/* ========================================================================= */

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
};

#endif // TEXTSCRIPTWIDGET_H

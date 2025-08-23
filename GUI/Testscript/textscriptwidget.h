
#ifndef TEXTSCRIPTWIDGET_H
#define TEXTSCRIPTWIDGET_H

#include <QWidget>
#include <QListWidget>
#include <QDir>
#include <QMenu>
#include <QAction>
#include <QPushButton>
#include <QHBoxLayout>
#include <QVBoxLayout>

class TextScriptItemWidget : public QWidget {
    Q_OBJECT
public:
    explicit TextScriptItemWidget(const QString &fileName, const QString &filePath, QWidget *parent = nullptr);
    QPushButton *playButton;
    QPushButton *pauseButton;
    void setActiveButton(const QString &state);

signals:
    void playClicked(const QString &filePath);
    void pauseClicked(const QString &filePath);
};

class TextScriptWidget : public QWidget {
    Q_OBJECT

public:
    explicit TextScriptWidget(QWidget *parent = nullptr);

signals:
    void runScript(const QString &filePath);
    void pauseScript(const QString &filePath);
    void renameScript(const QString &filePath, const QString &newName);
    void removeScript(const QString &filePath);

private slots:
    void handleCustomContextMenu(const QPoint &pos);
    void handleRenameAction();
    void handleRemoveAction();
    void handleEditAction(); // New slot for Edit action
    void handlePlayClicked(const QString &filePath);
    void handlePauseClicked(const QString &filePath);
    void handleAddScriptButtonClicked();

private:
    void loadScriptFiles(const QString &directoryPath);
    void updateStatusIcon(QListWidgetItem *item, const QString &status);
    QListWidget *fileListWidget;
    QMap<QString, QString> fileStatus;
    QMap<QString, QString> activeButtonState;
    QPushButton *addScriptButton;
};

#endif // TEXTSCRIPTWIDGET_H

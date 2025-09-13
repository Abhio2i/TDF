
#ifndef RUNTIMETOOLBAR_H
#define RUNTIMETOOLBAR_H
#include <QToolBar>
#include <QAction>
#include <QSlider>
#include <QLabel>
#include "GUI/Timing/timingdialog.h"

class RuntimeToolBar : public QToolBar
{
    Q_OBJECT
public:
    explicit RuntimeToolBar(QWidget *parent = nullptr);
signals:
    void startTriggered();
    void pauseTriggered();
    void stopTriggered();
    void nextStepTriggered();
    void speedChanged(int speed);
    void replayTriggered();
    void bookmarkTriggered();
    void bookmarkCommentSubmitted(QString comment);
    void loggerTriggered();
    void startRecording();
    void stopRecording();
    void replayRecording(const QString &filePath);
    void eventTypesSelected(QStringList eventTypes);
public:
    void onElapsedTime(float time);
private:
    QAction *startAction;
    QAction *pauseAction;
    QAction *stopAction;
    QAction *nextStepAction;
    QAction *bookmarkAction;
    QSlider *speedSlider;
    QAction *replayAction;
    QAction *timingAction;
    QAction *loggerAction;
    QLabel *timeLabel;
    QTimer *timer;
    float elapsedSeconds;
    TimingDialog *timingDialog = nullptr;
    QPixmap withWhiteBg(const QString &iconPath);
    void createActions();
    void setupToolBar();
    void highlightAction(QAction *action);
    void updateTimeDisplay();
};
#endif // RUNTIMETOOLBAR_H

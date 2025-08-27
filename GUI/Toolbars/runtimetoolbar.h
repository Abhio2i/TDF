
#ifndef RuntimeToolBar_H
#define RuntimeToolBar_H

#include <QToolBar>
#include <QAction>
#include <QSlider>
#include <QLabel>

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

public:
    void onElapsedTime(float time);
private:
    QAction *startAction;
    QAction *pauseAction;
    QAction *stopAction;
    QAction *nextStepAction;
    QSlider *speedSlider;
    QAction *replayAction;
    QLabel *timeLabel;
    QTimer *timer;
    float elapsedSeconds;
    QPixmap withWhiteBg(const QString &iconPath);
    void createActions();
    void setupToolBar();
    void highlightAction(QAction *action);
    void updateTimeDisplay();
};

#endif

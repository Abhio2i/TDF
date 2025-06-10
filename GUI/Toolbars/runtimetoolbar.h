
#ifndef RuntimeToolBar_H
#define RuntimeToolBar_H

#include <QToolBar>
#include <QAction>
#include <QSlider>

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

private:
    QAction *startAction;
    QAction *pauseAction;
    QAction *stopAction;
    QAction *nextStepAction;
    QSlider *speedSlider;

    QPixmap withWhiteBg(const QString &iconPath);
    void createActions();
    void setupToolBar();
    void highlightAction(QAction *action);
};

#endif

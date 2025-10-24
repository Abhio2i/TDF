
#include "runtimetoolbar.h"
#include <QIcon>
#include <QPainter>
#include <QPixmap>
#include <QToolButton>
#include <QSizePolicy>
#include <QToolTip>
#include <QLabel>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QGridLayout>
#include <QStyle>
#include <QStyleOptionSlider>
#include <QTimer>
#include <QDebug>
#include <QDialog>
#include <QLineEdit>
#include <QPushButton>
#include <GUI/Timing/timingdialog.h>

const QSize ICON_SIZE(24, 24);

QPixmap RuntimeToolBar::withWhiteBg(const QString &iconPath)
{
    QPixmap pixmap(iconPath);
    if (pixmap.isNull()) return QPixmap();
    QPixmap newPixmap(pixmap.size());
    newPixmap.fill(Qt::gray);
    QPainter painter(&newPixmap);
    painter.drawPixmap(0, 0, pixmap);
    painter.end();
    return newPixmap;
}

RuntimeToolBar::RuntimeToolBar(QWidget *parent) : QToolBar(parent)
{
    elapsedSeconds = 0;
    createActions();
    setupToolBar();
}

void RuntimeToolBar::createActions()
{
    this->setIconSize(ICON_SIZE);
    startAction = new QAction(QIcon(withWhiteBg(":/icons/images/play.png")), tr("Start"), this);
    startAction->setCheckable(true);
    startAction->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_P));
    connect(startAction, &QAction::triggered, this, [this](bool) {
        highlightAction(startAction);
        emit startTriggered();
        if (timingDialog && timingDialog->isVisible()) {
            timingDialog->startGraphSimulation();
        }
    });

    pauseAction = new QAction(QIcon(withWhiteBg(":/icons/images/pause.png")), tr("Pause"), this);
    pauseAction->setCheckable(true);
    connect(pauseAction, &QAction::triggered, this, [=]() {
        highlightAction(pauseAction);
        emit pauseTriggered();
    });

    stopAction = new QAction(QIcon(withWhiteBg(":/icons/images/stop.png")), tr("Stop"), this);
    stopAction->setCheckable(true);
    stopAction->setShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_P));
    connect(stopAction, &QAction::triggered, this, [=]() {
        highlightAction(stopAction);
        emit stopTriggered();
    });

    replayAction = new QAction(QIcon(withWhiteBg(":/icons/images/replay.png")), tr("Replay"), this);
    replayAction->setCheckable(true);
    connect(replayAction, &QAction::triggered, this, [=]() {
        highlightAction(replayAction);
        emit replayTriggered();
    });

    nextStepAction = new QAction(QIcon(withWhiteBg(":/icons/images/step.png")), tr("Next Step"), this);
    nextStepAction->setCheckable(false);
    connect(nextStepAction, &QAction::triggered, this, [=]() {
        emit nextStepTriggered();
    });

    // bookmarkAction = new QAction(QIcon(withWhiteBg(":/icons/images/star.png")), tr("Bookmark"), this);
    // bookmarkAction->setCheckable(false);
    // connect(bookmarkAction, &QAction::triggered, this, [this]() {
    //     emit bookmarkTriggered();
    //     QDialog *bookmarkDialog = new QDialog(this);
    //     bookmarkDialog->setWindowTitle(tr("Add Bookmark Comment"));
    //     QVBoxLayout *layout = new QVBoxLayout(bookmarkDialog);
    //     QLineEdit *commentInput = new QLineEdit(bookmarkDialog);
    //     commentInput->setPlaceholderText(tr("Enter your comment"));
    //     QPushButton *okButton = new QPushButton(tr("OK"), bookmarkDialog);
    //     layout->addWidget(commentInput);
    //     layout->addWidget(okButton);
    //     bookmarkDialog->setLayout(layout);
    //     connect(okButton, &QPushButton::clicked, this, [this, commentInput, bookmarkDialog]() {
    //         QString comment = commentInput->text().trimmed();
    //         if (!comment.isEmpty()) {
    //             emit bookmarkCommentSubmitted(comment);
    //         }
    //         bookmarkDialog->accept();
    //     });
    //     bookmarkDialog->exec();
    // });

    timingAction = new QAction(QIcon(withWhiteBg(":/icons/images/timing.png")), tr("Timing Info"), this);
    timingAction->setCheckable(false);
    connect(timingAction, &QAction::triggered, this, [this]() {
        if (!timingDialog) {
            timingDialog = new TimingDialog(this);
        }
        timingDialog->show();
    });

loggerAction = new QAction(QIcon(withWhiteBg(":/icons/images/audit.png")), tr("Logger"), this);
loggerAction->setCheckable(true);
loggerAction->setObjectName("loggerAction");
connect(loggerAction, &QAction::triggered, this, [this](bool checked) {
    emit loggerTriggered(checked); // Emit with checked state
    highlightAction(checked ? loggerAction : nullptr);
});


    // New action for toggling RadarDisplay (initially unchecked)
    radarToggleAction = new QAction(QIcon(withWhiteBg(":/icons/images/database (1).png")), tr("Toggle Radar Display"), this);
    radarToggleAction->setObjectName("radarToggleAction");
    radarToggleAction->setCheckable(true);
    radarToggleAction->setChecked(false); // Initially unchecked (RadarDisplay hidden)
    connect(radarToggleAction, &QAction::triggered, this, [=](bool checked) {
        qDebug() << "RadarDisplay toggle triggered, checked:" << checked;
        emit radarDisplayToggled();
    });

    QWidget *speedControlWidget = new QWidget(this);
    QHBoxLayout *speedLayout = new QHBoxLayout(speedControlWidget);
    speedLayout->setContentsMargins(0, 0, 0, 0);
    speedLayout->setSpacing(5);
    QToolButton *speedIcon = new QToolButton(this);
    speedIcon->setIcon(QIcon(withWhiteBg(":/icons/images/speed.png")));
    speedIcon->setToolTip(tr("Adjust Speed"));
    speedIcon->setStyleSheet("QToolButton { border: none; padding: 0px; }");
    speedIcon->setIconSize(ICON_SIZE);
    speedSlider = new QSlider(Qt::Horizontal, this);
    speedSlider->setRange(1, 10);
    speedSlider->setValue(1);
    speedSlider->setMinimumWidth(100);
    speedSlider->setMaximumWidth(150);
    speedSlider->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
    speedSlider->setToolTip(tr("Current Speed: %1").arg(speedSlider->value()));
    timeLabel = new QLabel("00:00:00", this);
    timeLabel->setStyleSheet("QLabel { font-family: monospace; padding: 0 5px; }");
    timeLabel->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, [=]() {
        elapsedSeconds++;
        updateTimeDisplay();
    });
    speedLayout->addWidget(speedIcon);
    speedLayout->addWidget(speedSlider);
    speedLayout->addWidget(timeLabel);
    speedControlWidget->setLayout(speedLayout);
    connect(speedSlider, &QSlider::valueChanged, this, [=](int value) {
        speedSlider->setToolTip(tr("Speed: %1").arg(value));
        QStyleOptionSlider opt;
        opt.initFrom(speedSlider);
        opt.minimum = speedSlider->minimum();
        opt.maximum = speedSlider->maximum();
        opt.sliderPosition = value;
        opt.sliderValue = value;
        opt.orientation = speedSlider->orientation();
        QRect handleRect = speedSlider->style()->subControlRect(
            QStyle::CC_Slider, &opt, QStyle::SC_SliderHandle, speedSlider);
        QPoint tooltipPos = speedSlider->mapToGlobal(
            QPoint(handleRect.center().x(), handleRect.top() - 10));
        QToolTip::showText(tooltipPos, QString::number(value), speedSlider);
        emit speedChanged(value);
    });
    speedSlider->setAttribute(Qt::WA_AlwaysShowToolTips);
}
void RuntimeToolBar::setupToolBar()
{
    QToolButton* btn;
    for (QAction* action : this->actions()) {
        if ((btn = qobject_cast<QToolButton*>(widgetForAction(action)))) {
            btn->setFixedSize(ICON_SIZE.width() + 10, ICON_SIZE.height() + 10);
        }
    }
    addAction(startAction);
    addAction(pauseAction);
    addAction(stopAction);
    addAction(replayAction);
    addAction(nextStepAction);
    // addAction(bookmarkAction);
    addAction(timingAction);
    addAction(loggerAction);
    addAction(radarToggleAction); // Add radar toggle action
    addSeparator();
    addWidget(speedSlider->parentWidget());
    addSeparator();
}

void RuntimeToolBar::highlightAction(QAction *activeAction)
{
    QList<QAction*> actions = { startAction, pauseAction, stopAction, replayAction, loggerAction, radarToggleAction };
    for (QAction *action : actions) {
        QWidget *btn = widgetForAction(action);
        if (!btn) continue;
        if (action == activeAction) {
            btn->setStyleSheet("QToolButton { background-color: #d0e0ff; border: 1px solid #5070ff; border-radius: 4px; }");
        } else {
            btn->setStyleSheet("");
        }
    }
}

void RuntimeToolBar::onElapsedTime(float time)
{
    elapsedSeconds += time;
    updateTimeDisplay();
}

void RuntimeToolBar::updateTimeDisplay()
{
    int totalSeconds = static_cast<int>(elapsedSeconds);
    int hours = totalSeconds / 3600;
    int minutes = (totalSeconds % 3600) / 60;
    int seconds = totalSeconds % 60;
    timeLabel->setText(QString("%1:%2:%3")
                           .arg(hours, 2, 10, QChar('0'))
                           .arg(minutes, 2, 10, QChar('0'))
                           .arg(seconds, 2, 10, QChar('0')));
}

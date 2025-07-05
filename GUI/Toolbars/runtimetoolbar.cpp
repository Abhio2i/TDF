
#include "runtimetoolbar.h"
#include <QIcon>
#include <QPainter>
#include <QPixmap>
#include <QToolButton>
#include <QSizePolicy>
#include <QToolTip>
#include <QLabel>
#include <QHBoxLayout>
#include <QStyle>
#include <QStyleOptionSlider>

// Define a constant icon size
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
    createActions();
    setupToolBar();
}

void RuntimeToolBar::createActions()
{
    // Set icon size for all actions
    this->setIconSize(ICON_SIZE);

    startAction = new QAction(QIcon(withWhiteBg(":/icons/images/play.png")), tr("Start"), this);
    startAction->setCheckable(true);
    startAction->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_P));
    connect(startAction, &QAction::triggered, this, [=]() {
        highlightAction(startAction);
        emit startTriggered();
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

    // Create speed control widgets
    QWidget *speedControlWidget = new QWidget(this);
    QHBoxLayout *speedLayout = new QHBoxLayout(speedControlWidget);
    speedLayout->setContentsMargins(0, 0, 0, 0);
    speedLayout->setSpacing(5);

    // Add speed icon - now using the same size as other icons
    QToolButton *speedIcon = new QToolButton(this);
    speedIcon->setIcon(QIcon(withWhiteBg(":/icons/images/speed.png")));
    speedIcon->setToolTip(tr("Adjust Speed"));
    speedIcon->setStyleSheet("QToolButton { border: none; padding: 0px; }");
    speedIcon->setIconSize(ICON_SIZE);

    // Create and configure the speed slider
    speedSlider = new QSlider(Qt::Horizontal, this);
    speedSlider->setRange(1, 10);
    speedSlider->setValue(5);
    speedSlider->setMinimumWidth(100);
    speedSlider->setMaximumWidth(150);
    speedSlider->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
    speedSlider->setToolTip(tr("Current Speed: %1").arg(speedSlider->value()));

    // Add widgets to speed control layout
    speedLayout->addWidget(speedIcon);
    speedLayout->addWidget(speedSlider);
    speedControlWidget->setLayout(speedLayout);

    // Connect value changed signal with proper tooltip display
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
    // Set uniform button sizes
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

    // Add spacer and speed control widget
    addSeparator();
    addWidget(speedSlider->parentWidget());

    addSeparator();
}

void RuntimeToolBar::highlightAction(QAction *activeAction)
{
    QList<QAction*> actions = { startAction, pauseAction, stopAction };

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



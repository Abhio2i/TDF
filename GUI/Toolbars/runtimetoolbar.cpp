/* =============================================================================
 * FILE:         runtimetoolbar.cpp
 * MODULE:       Runtime Toolbar
 * PROJECT:      Indigenous Scenario and Sensor Simulation Toolkit (ISSST)
 * ORGANISATION: Oxygen 2 Innovation (O2I).
 * STANDARD:     RTCA DO-178C / ED-12C, DAL B
 * COVERAGE:     Branch / Decision Coverage required (100% true/false paths)
 *
 * DESCRIPTION:  Implements the RuntimeToolBar class which provides a toolbar for
 *               runtime control and monitoring. Includes actions for start, pause,
 *               stop, next step, reset, timing graph, logger, radar toggle, and
 *               a speed slider with time label. Supports simulation state
 *               (STOPPED, RUNNING, PAUSED), snapshot storage, event filtering,
 *               and emits signals for simulation control and timing/logging.
 *
 * REQUIREMENTS: Implements REQ-RUNTIMETOOLBAR-010 through REQ-RUNTIMETOOLBAR-018
 *
 * AUTHOR:       Arti Rajpoot
 * REVIEWED BY:  [Reviewer Name], [Review Date] — SPR-RUNTIMETOOLBAR-001
 *
 *
 * COPYRIGHT:    Oxygen 2 Innovation (O2I). All rights reserved.
 *               Restricted circulation — defence simulation use only.
 * =============================================================================
 */

#include "runtimetoolbar.h"
#include "runtimetoolbar-styles.h"
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
#include <QApplication>
#include "GUI/Timing/graphwidget.h"
#include "GUI/Editors/runtimeeditor.h"

const QSize ICON_SIZE(20, 20);

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

// Constructor for RuntimeToolBar
RuntimeToolBar::RuntimeToolBar(QWidget *parent) : QToolBar(parent)
{
    setWindowTitle("Runtime ToolBar");
    setStyleSheet(RuntimeToolbarStyles::Toolbar);
    QToolTip::setPalette(QPalette());
    setToolTipDuration(2000);
    elapsedSeconds = 0;
    currentState = STOPPED;
    blinkState = false;
    createActions();
    setupToolBar();


}
void RuntimeToolBar::Init(){
    elapsedSeconds = 0;
    currentState = STOPPED;
    blinkState = false;
    startAction->setIcon(QIcon(withWhiteBg(":/icons/images/play.png")));
    startAction->setText(tr("Start"));
    startAction->setChecked(false);
    highlightAction(nullptr);
    updateTimeDisplay();
    updateStatusDisplay();
}
// Create all toolbar actions and their connections
void RuntimeToolBar::createActions()
{
    this->setIconSize(ICON_SIZE);


    startAction = new QAction(QIcon(withWhiteBg(":/icons/images/play.png")), tr("Start"), this);
    startAction->setCheckable(true);
    startAction->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_P));
    connect(startAction, &QAction::triggered, this, [this](bool) {
        if (currentState != RUNNING) {
            // Currently stopped/paused → go RUNNING, show pause icon
            startAction->setIcon(QIcon(withWhiteBg(":/icons/images/pause.png")));
            startAction->setText(tr("Pause"));
            startAction->setChecked(true);
            highlightAction(startAction);
            setSimulationState(RUNNING);
            emit startTriggered();
        } else {
            // Currently running → go PAUSED, show play icon
            startAction->setIcon(QIcon(withWhiteBg(":/icons/images/play.png")));
            startAction->setText(tr("Start"));
            startAction->setChecked(false);
            highlightAction(nullptr);
            setSimulationState(PAUSED);
            emit pauseTriggered();
        }
    });

    // Pause Action — kept for signal compatibility
    pauseAction = new QAction(QIcon(withWhiteBg(":/icons/images/pause.png")), tr("Pause"), this);
    pauseAction->setCheckable(true);
    pauseAction->setVisible(false);
    // Stop Action (Stop button)
    stopAction = new QAction(QIcon(withWhiteBg(":/icons/images/stop.png")), tr("Stop"), this);
    stopAction->setCheckable(true);
    stopAction->setShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_P));
    connect(stopAction, &QAction::triggered, this, [=]() {
        highlightAction(stopAction);
        setSimulationState(STOPPED);
        emit stopTriggered();
    });
    // Reset Action
    resetAction = new QAction(QIcon(withWhiteBg(":/icons/images/reset.png")), tr("Reset"), this);
    resetAction->setCheckable(false);
    resetAction->setToolTip(tr("Reset simulation to initial state"));
    connect(resetAction, &QAction::triggered, this, [this]() {
        // Stop simulation first
        setSimulationState(STOPPED);
        emit stopTriggered();

        // Reset time
        elapsedSeconds = 0;
        updateTimeDisplay();

        // Restore snapshot and notify
        emit resetTriggered();
    });

    // Next Step Action (Step button)
    nextStepAction = new QAction(QIcon(withWhiteBg(":/icons/images/step.png")), tr("Next Step"), this);
    nextStepAction->setCheckable(false);
    connect(nextStepAction, &QAction::triggered, this, [=]() {
        emit nextStepTriggered();
    });

    // Timing Graph Action (Timing button)
    timingAction = new QAction(QIcon(withWhiteBg(":/icons/images/timing.png")), tr("Timing Graph"), this);
    timingAction->setCheckable(false);
    connect(timingAction, &QAction::triggered, this, [this]() {
        GraphWidget* existingGraph = nullptr;
        QWidgetList topLevelWidgets = QApplication::topLevelWidgets();
        for (QWidget* widget : topLevelWidgets) {
            GraphWidget* graph = qobject_cast<GraphWidget*>(widget);
            if (graph) {
                existingGraph = graph;
                break;
            }
        }

        if (existingGraph) {
            existingGraph->show();
            existingGraph->raise();
            existingGraph->activateWindow();
        } else {
            QWidget* topParent = this->window();
            GraphWidget* graph = new GraphWidget(topParent);
            graph->setWindowTitle("Timing Graph");
            graph->setWindowFlags(Qt::Tool |
                                  Qt::WindowStaysOnTopHint |
                                  Qt::WindowCloseButtonHint |
                                  Qt::WindowTitleHint |
                                  Qt::CustomizeWindowHint);
            graph->setAttribute(Qt::WA_ShowWithoutActivating);
            graph->resize(1000, 600);
            graph->show();
            graph->raise();
            graph->activateWindow();

            QWidget* parentWidget = this->parentWidget();
            while (parentWidget) {
                RuntimeEditor* editor = qobject_cast<RuntimeEditor*>(parentWidget);
                if (editor) {
                    if (editor->hierarchy) {
                        graph->setHierarchy(editor->hierarchy);
                    }
                    if (editor->simulation) {
                        connect(editor->simulation, &Simulation::Render, graph, &GraphWidget::refresh);
                    }
                    break;
                }
                parentWidget = parentWidget->parentWidget();
            }
        }
    });

    // Logger Action (Audit button)
    loggerAction = new QAction(QIcon(withWhiteBg(":/icons/images/audit.png")), tr("Logger"), this);
    loggerAction->setCheckable(true);
    loggerAction->setObjectName("loggerAction");
    connect(loggerAction, &QAction::triggered, this, [this](bool checked) {
        emit loggerTriggered(checked);
        highlightAction(checked ? loggerAction : nullptr);
    });

    // Radar Display Toggle Action (Database icon)
    radarToggleAction = new QAction(QIcon(withWhiteBg(":/icons/images/database (1).png")), tr("Toggle Radar Display"), this);
    radarToggleAction->setObjectName("radarToggleAction");
    radarToggleAction->setCheckable(true);
    radarToggleAction->setChecked(false);
    connect(radarToggleAction, &QAction::triggered, this, [=](bool checked) {
        emit radarDisplayToggled();
    });

    // Create speed control widget with slider and time display
    QWidget *speedControlWidget = new QWidget(this);
    speedControlWidget->setStyleSheet("background-color: transparent;");

    QHBoxLayout *speedLayout = new QHBoxLayout(speedControlWidget);
    speedLayout->setContentsMargins(0, 0, 0, 0);
    speedLayout->setSpacing(5);

    // Speed icon button
    QToolButton *speedIcon = new QToolButton(this);
    speedIcon->setIcon(QIcon(withWhiteBg(":/icons/images/speed.png")));
    speedIcon->setToolTip(tr("Adjust Speed"));
    // speedIcon->setStyleSheet(RuntimeToolbarStyles::SpeedIconButton);
    speedIcon->setStyleSheet(RuntimeToolbarStyles::SpeedIconButton + R"(
    QToolTip {
        background-color: #1A3652;
        color: white;
        border: 1px solid #0078D4;
        border-radius: 3px;
        padding: 4px 8px;
        font-size: 11px;
    }
)");
    speedIcon->setIconSize(ICON_SIZE);

    // Speed slider for adjusting simulation speed
    speedSlider = new QSlider(Qt::Horizontal, this);
    speedSlider->setRange(1, 10);
    speedSlider->setValue(1);
    speedSlider->setMinimumWidth(100);
    speedSlider->setMaximumWidth(150);
    speedSlider->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
    speedSlider->setToolTip(tr("Current Speed: %1").arg(speedSlider->value()));
    // speedSlider->setStyleSheet(RuntimeToolbarStyles::SpeedSlider);
    speedSlider->setStyleSheet(RuntimeToolbarStyles::SpeedSlider + R"(
    QToolTip {
        background-color: #1A3652;
        color: white;
        border: 1px solid #0078D4;
        border-radius: 3px;
        padding: 4px 8px;
        font-size: 11px;
    }
)");

    // Time display label
    timeLabel = new QLabel("00:00:00", this);
    timeLabel->setStyleSheet(RuntimeToolbarStyles::TimeLabel);
    timeLabel->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    timeLabel->installEventFilter(this);

    // Simulation status bar
    simulationStatusLabel = new QLabel("STOPPED", this);
    simulationStatusLabel->setStyleSheet(RuntimeToolbarStyles::StatusStopped);
    simulationStatusLabel->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    simulationStatusLabel->setMinimumWidth(80);
    simulationStatusLabel->setAlignment(Qt::AlignCenter);
    simulationStatusLabel->setToolTip("Simulation Status");
    simulationStatusLabel->setVisible(false);


    speedLayout->addWidget(speedIcon);
    speedLayout->addWidget(speedSlider);
    speedLayout->addWidget(timeLabel);
    speedLayout->addWidget(simulationStatusLabel);
    speedControlWidget->setLayout(speedLayout);

    timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, [=]() {
        //elapsedSeconds++;
        updateTimeDisplay();
    });

    blinkTimer = new QTimer(this);
    connect(blinkTimer, &QTimer::timeout, this, &RuntimeToolBar::updateSimulationStatus);

    // Connect slider value change
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

bool RuntimeToolBar::eventFilter(QObject *obj, QEvent *event)
{
    if (obj == timeLabel && event->type() == QEvent::MouseButtonPress) {
        onTimeLabelClicked();
        return true;
    }
    return QToolBar::eventFilter(obj, event);
}

// Setup toolbar layout by adding all actions and widgets
void RuntimeToolBar::setupToolBar()
{
    QToolButton* btn;
    for (QAction* action : this->actions()) {
        if ((btn = qobject_cast<QToolButton*>(widgetForAction(action)))) {
            btn->setFixedSize(ICON_SIZE.width() + 10, ICON_SIZE.height() + 10);
            btn->setStyleSheet(RuntimeToolbarStyles::ToolbarButton);
        }
    }

    addAction(startAction);
      addAction(resetAction);
    addAction(nextStepAction);
    addAction(timingAction);
    addAction(loggerAction);
    addAction(radarToggleAction);
    addSeparator();
    addWidget(speedSlider->parentWidget());
    addSeparator();
}

// Highlight the currently active action
void RuntimeToolBar::highlightAction(QAction *activeAction)
{
    QList<QAction*> actions = { startAction, stopAction, loggerAction, radarToggleAction};
    for (QAction *action : actions) {
        QWidget *btn = widgetForAction(action);
        if (!btn) continue;
        if (action == activeAction) {
            btn->setStyleSheet(RuntimeToolbarStyles::ToolbarButtonHighlighted);
        } else {
            btn->setStyleSheet(RuntimeToolbarStyles::ToolbarButton);
        }
    }
}

// Update elapsed time from external source
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

void RuntimeToolBar::onTimeLabelClicked()
{
    int totalSeconds = static_cast<int>(elapsedSeconds);
    int hours = totalSeconds / 3600;
    int minutes = (totalSeconds % 3600) / 60;
    int seconds = totalSeconds % 60;

    QDialog *dialog = new QDialog(this);
    dialog->setWindowTitle("Set Time");
    dialog->setModal(true);
    dialog->setStyleSheet(RuntimeToolbarStyles::Dialog);

    QVBoxLayout *layout = new QVBoxLayout(dialog);
    QHBoxLayout *timeLayout = new QHBoxLayout();
    QLineEdit *hoursEdit = new QLineEdit(dialog);
    hoursEdit->setPlaceholderText("HH");
    hoursEdit->setText(QString::number(hours));
    hoursEdit->setMaxLength(2);
    hoursEdit->setFixedWidth(50);

    QLineEdit *minutesEdit = new QLineEdit(dialog);
    minutesEdit->setPlaceholderText("MM");
    minutesEdit->setText(QString::number(minutes));
    minutesEdit->setMaxLength(2);
    minutesEdit->setFixedWidth(50);

    QLineEdit *secondsEdit = new QLineEdit(dialog);
    secondsEdit->setPlaceholderText("SS");
    secondsEdit->setMaxLength(2);
    secondsEdit->setFixedWidth(50);
    secondsEdit->setText(QString::number(seconds));

    QLabel *timePrompt = new QLabel("Time:", dialog);
    timePrompt->setStyleSheet("color: white;");

    timeLayout->addWidget(timePrompt);
    timeLayout->addWidget(hoursEdit);
    timeLayout->addWidget(minutesEdit);
    timeLayout->addWidget(secondsEdit);
    layout->addLayout(timeLayout);

    QHBoxLayout *buttonLayout = new QHBoxLayout();
    QPushButton *okButton = new QPushButton("OK", dialog);
    QPushButton *cancelButton = new QPushButton("Cancel", dialog);
    okButton->setStyleSheet(RuntimeToolbarStyles::PushButton);
    cancelButton->setStyleSheet(RuntimeToolbarStyles::PushButton);

    buttonLayout->addStretch();
    buttonLayout->addWidget(okButton);
    buttonLayout->addWidget(cancelButton);
    layout->addLayout(buttonLayout);

    connect(okButton, &QPushButton::clicked, dialog, &QDialog::accept);
    connect(cancelButton, &QPushButton::clicked, dialog, &QDialog::reject);

    if (dialog->exec() == QDialog::Accepted) {
        int hours = hoursEdit->text().toInt();
        int minutes = minutesEdit->text().toInt();
        int seconds = secondsEdit->text().toInt();
        float totalSeconds = hours * 3600 + minutes * 60 + seconds;
        if(totalSeconds>elapsedSeconds){
            elapsedSeconds = totalSeconds;
            updateTimeDisplay();
            emit timeChanged(totalSeconds);
        }
    }
    dialog->deleteLater();
}


void RuntimeToolBar::setSimulationState(SimulationState state)
{
    currentState = state;
    updateStatusDisplay();
    emit simulationStateChanged(state);
    if (state == RUNNING) {
        if (!timer->isActive()) {
            timer->start(1000);
        }
        blinkTimer->start(500);
        simulationStatusLabel->setVisible(true);
        startAction->setIcon(QIcon(withWhiteBg(":/icons/images/pause.png")));
        startAction->setText(tr("Pause"));
        startAction->setChecked(true);
    } else if (state == PAUSED) {
        timer->stop();
        blinkTimer->start(1000);
        simulationStatusLabel->setVisible(true);
        startAction->setIcon(QIcon(withWhiteBg(":/icons/images/play.png")));
        startAction->setText(tr("Start"));
        startAction->setChecked(false);
    } else { // STOPPED
        timer->stop();
        blinkTimer->stop();
        blinkState = false;
        simulationStatusLabel->setVisible(false);
        updateStatusDisplay();
        startAction->setIcon(QIcon(withWhiteBg(":/icons/images/play.png")));
        startAction->setText(tr("Start"));
        startAction->setChecked(false);
        highlightAction(nullptr);
    }
}

void RuntimeToolBar::updateStatusDisplay()
{
    QString text;
    QString styleSheet;

    switch(currentState) {
    case RUNNING:
        text = "RUNNING";
        if (blinkState) {
            styleSheet = RuntimeToolbarStyles::StatusRunningBlink;
        } else {
            styleSheet = RuntimeToolbarStyles::StatusRunning;
        }
        simulationStatusLabel->setVisible(true);
        break;

    case PAUSED:
        text = "PAUSED";
        if (blinkState) {
            styleSheet = RuntimeToolbarStyles::StatusPausedBlink;
        } else {
            styleSheet = RuntimeToolbarStyles::StatusPaused;
        }
        simulationStatusLabel->setVisible(true);
        break;

    case STOPPED:
        simulationStatusLabel->setVisible(false);
        return;
    }

    simulationStatusLabel->setText(text);
    simulationStatusLabel->setStyleSheet(styleSheet);
}

void RuntimeToolBar::updateSimulationStatus()
{
    blinkState = !blinkState;
    updateStatusDisplay();
}
void RuntimeToolBar::storeSnapshot(const QJsonObject& hierarchySnapshot)
{
    m_initialSnapshot = hierarchySnapshot;
}

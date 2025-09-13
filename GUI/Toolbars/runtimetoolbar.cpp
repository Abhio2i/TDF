
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
#include <QCheckBox>
#include <QListWidget>
#include <QStandardPaths>
#include <QDir>
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

    bookmarkAction = new QAction(QIcon(withWhiteBg(":/icons/images/star.png")), tr("Bookmark"), this);
    bookmarkAction->setCheckable(false);
    connect(bookmarkAction, &QAction::triggered, this, [this]() {
        emit bookmarkTriggered();
        QDialog *bookmarkDialog = new QDialog(this);
        bookmarkDialog->setWindowTitle(tr("Add Bookmark Comment"));
        QVBoxLayout *layout = new QVBoxLayout(bookmarkDialog);
        QLineEdit *commentInput = new QLineEdit(bookmarkDialog);
        commentInput->setPlaceholderText(tr("Enter your comment"));
        QPushButton *okButton = new QPushButton(tr("OK"), bookmarkDialog);
        layout->addWidget(commentInput);
        layout->addWidget(okButton);
        bookmarkDialog->setLayout(layout);
        connect(okButton, &QPushButton::clicked, this, [this, commentInput, bookmarkDialog]() {
            QString comment = commentInput->text().trimmed();
            if (!comment.isEmpty()) {
                emit bookmarkCommentSubmitted(comment);
            }
            bookmarkDialog->accept();
        });
        bookmarkDialog->exec();
    });

    timingAction = new QAction(QIcon(withWhiteBg(":/icons/images/timing.png")), tr("Timing Info"), this);
    timingAction->setCheckable(false);
    connect(timingAction, &QAction::triggered, this, [this]() {
        if (!timingDialog) {
            timingDialog = new TimingDialog(this);
        }
        timingDialog->show();
    });

    loggerAction = new QAction(QIcon(withWhiteBg(":/icons/images/audit.png")), tr("Logger"), this);
    loggerAction->setCheckable(false);
    connect(loggerAction, &QAction::triggered, this, [this]() {
        emit loggerTriggered();
        QDialog *loggerDialog = new QDialog(this);
        loggerDialog->setWindowTitle(tr("Logger Control"));
        loggerDialog->setAttribute(Qt::WA_DeleteOnClose);
        QGridLayout *mainLayout = new QGridLayout(loggerDialog);

        // Left side: Event types
        QVBoxLayout *eventLayout = new QVBoxLayout();
        QLabel *eventLabel = new QLabel(tr("Select Events to Record:"), loggerDialog);
        QCheckBox *actionsCheckBox = new QCheckBox(tr("Actions"), loggerDialog);
        QCheckBox *waypointsCheckBox = new QCheckBox(tr("Waypoints"), loggerDialog);
        QCheckBox *engagementsCheckBox = new QCheckBox(tr("Engagements"), loggerDialog);
        actionsCheckBox->setChecked(true);
        waypointsCheckBox->setChecked(true);
        engagementsCheckBox->setChecked(true);
        eventLayout->addWidget(eventLabel);
        eventLayout->addWidget(actionsCheckBox);
        eventLayout->addWidget(waypointsCheckBox);
        eventLayout->addWidget(engagementsCheckBox);
        eventLayout->addStretch();

        // Right side: Recording controls
        QVBoxLayout *controlLayout = new QVBoxLayout();
        QPushButton *startRecordingButton = new QPushButton(tr("Start Recording"), loggerDialog);
        QPushButton *stopRecordingButton = new QPushButton(tr("Stop Recording"), loggerDialog);
        QLabel *statusLabel = new QLabel(tr("No recording active"), loggerDialog);
        statusLabel->setStyleSheet("QLabel { font-family: monospace; padding: 5px; }");

        // Recording list and replay
        QLabel *recordingsLabel = new QLabel(tr("Available Recordings:"), loggerDialog);
        QListWidget *recordingsList = new QListWidget(loggerDialog);
        recordingsList->setSelectionMode(QAbstractItemView::SingleSelection);
        QPushButton *replayButton = new QPushButton(tr("Replay Selected"), loggerDialog);
        replayButton->setEnabled(false);

        // Populate recordings list
        QString recordingsDir = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation) + "/recordings";
        QDir dir(recordingsDir);
        if (!dir.exists()) {
            dir.mkpath(".");
        }
        QStringList filters;
        filters << "*.json";
        QStringList recordingFiles = dir.entryList(filters, QDir::Files, QDir::Time);
        for (const QString &file : recordingFiles) {
            recordingsList->addItem(file);
        }

        controlLayout->addWidget(startRecordingButton);
        controlLayout->addWidget(stopRecordingButton);
        controlLayout->addWidget(statusLabel);
        controlLayout->addWidget(recordingsLabel);
        controlLayout->addWidget(recordingsList);
        controlLayout->addWidget(replayButton);
        controlLayout->addStretch();

        // Add layouts to grid
        mainLayout->addLayout(eventLayout, 0, 0);
        mainLayout->addLayout(controlLayout, 0, 1);
        loggerDialog->setLayout(mainLayout);

        // Connect buttons to signals
        connect(startRecordingButton, &QPushButton::clicked, this, [this, statusLabel]() {
            emit startRecording();
            statusLabel->setText(tr("Recording active..."));
        });
        connect(stopRecordingButton, &QPushButton::clicked, this, [this, statusLabel, recordingsList, recordingsDir]() {
            emit stopRecording();
            statusLabel->setText(tr("Recording completed"));
            // Refresh recordings list
            recordingsList->clear();
            QStringList filters;
            filters << "*.json";
            QStringList recordingFiles = QDir(recordingsDir).entryList(filters, QDir::Files, QDir::Time);
            for (const QString &file : recordingFiles) {
                recordingsList->addItem(file);
            }
        });
        connect(recordingsList, &QListWidget::itemSelectionChanged, this, [replayButton, recordingsList]() {
            replayButton->setEnabled(recordingsList->selectedItems().count() > 0);
        });
        connect(replayButton, &QPushButton::clicked, this, [this, recordingsList, recordingsDir]() {
            if (recordingsList->selectedItems().count() > 0) {
                QString selectedFile = recordingsList->selectedItems().first()->text();
                QString filePath = recordingsDir + "/" + selectedFile;
                emit replayRecording(filePath);
            }
        });

        // Connect checkboxes to emit event types
        connect(actionsCheckBox, &QCheckBox::stateChanged, this, [this, actionsCheckBox, waypointsCheckBox, engagementsCheckBox]() {
            QStringList eventTypes;
            if (actionsCheckBox->isChecked()) eventTypes << "Actions";
            if (waypointsCheckBox->isChecked()) eventTypes << "Waypoints";
            if (engagementsCheckBox->isChecked()) eventTypes << "Engagements";
            emit eventTypesSelected(eventTypes);
        });
        connect(waypointsCheckBox, &QCheckBox::stateChanged, this, [this, actionsCheckBox, waypointsCheckBox, engagementsCheckBox]() {
            QStringList eventTypes;
            if (actionsCheckBox->isChecked()) eventTypes << "Actions";
            if (waypointsCheckBox->isChecked()) eventTypes << "Waypoints";
            if (engagementsCheckBox->isChecked()) eventTypes << "Engagements";
            emit eventTypesSelected(eventTypes);
        });
        connect(engagementsCheckBox, &QCheckBox::stateChanged, this, [this, actionsCheckBox, waypointsCheckBox, engagementsCheckBox]() {
            QStringList eventTypes;
            if (actionsCheckBox->isChecked()) eventTypes << "Actions";
            if (waypointsCheckBox->isChecked()) eventTypes << "Waypoints";
            if (engagementsCheckBox->isChecked()) eventTypes << "Engagements";
            emit eventTypesSelected(eventTypes);
        });

        loggerDialog->show();
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
    addAction(bookmarkAction);
    addAction(timingAction);
    addAction(loggerAction);
    addSeparator();
    addWidget(speedSlider->parentWidget());
    addSeparator();
}

void RuntimeToolBar::highlightAction(QAction *activeAction)
{
    QList<QAction*> actions = { startAction, pauseAction, stopAction, replayAction };
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

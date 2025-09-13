#include "logger.h"
#include <QVBoxLayout>
#include <QCheckBox>
#include <QPushButton>
#include <QLabel>

Logger::Logger(QWidget *parent) : QWidget(parent)
{
    setWindowTitle(tr("Logger Control"));
    setMinimumWidth(200);
    setupUi();
}

void Logger::setupUi()
{
    QVBoxLayout *layout = new QVBoxLayout(this);

    // Label
    QLabel *label = new QLabel(tr("Select Events to Record:"), this);

    // Checkboxes for event types
    actionsCheckBox = new QCheckBox(tr("Actions"), this);
    waypointsCheckBox = new QCheckBox(tr("Waypoints"), this);
    engagementsCheckBox = new QCheckBox(tr("Engagements"), this);
    actionsCheckBox->setChecked(true);
    waypointsCheckBox->setChecked(true);
    engagementsCheckBox->setChecked(true);

    // Buttons for recording control
    QPushButton *startRecordingButton = new QPushButton(tr("Start Recording"), this);
    QPushButton *stopRecordingButton = new QPushButton(tr("Stop Recording"), this);
    QPushButton *replayButton = new QPushButton(tr("Replay"), this);

    // Add widgets to layout
    layout->addWidget(label);
    layout->addWidget(actionsCheckBox);
    layout->addWidget(waypointsCheckBox);
    layout->addWidget(engagementsCheckBox);
    layout->addSpacing(10);
    layout->addWidget(startRecordingButton);
    layout->addWidget(stopRecordingButton);
    layout->addWidget(replayButton);
    layout->addStretch(); // Push content to top
    setLayout(layout);

    // Connect buttons to signals
    connect(startRecordingButton, &QPushButton::clicked, this, &Logger::startRecording);
    connect(stopRecordingButton, &QPushButton::clicked, this, &Logger::stopRecording);
    connect(replayButton, &QPushButton::clicked, this, &Logger::replayRecording);

    // Connect checkboxes to emit event types
    connect(actionsCheckBox, &QCheckBox::stateChanged, this, [this]() {
        QStringList eventTypes;
        if (actionsCheckBox->isChecked()) eventTypes << "Actions";
        if (waypointsCheckBox->isChecked()) eventTypes << "Waypoints";
        if (engagementsCheckBox->isChecked()) eventTypes << "Engagements";
        emit eventTypesSelected(eventTypes);
    });
    connect(waypointsCheckBox, &QCheckBox::stateChanged, this, [this]() {
        QStringList eventTypes;
        if (actionsCheckBox->isChecked()) eventTypes << "Actions";
        if (waypointsCheckBox->isChecked()) eventTypes << "Waypoints";
        if (engagementsCheckBox->isChecked()) eventTypes << "Engagements";
        emit eventTypesSelected(eventTypes);
    });
    connect(engagementsCheckBox, &QCheckBox::stateChanged, this, [this]() {
        QStringList eventTypes;
        if (actionsCheckBox->isChecked()) eventTypes << "Actions";
        if (waypointsCheckBox->isChecked()) eventTypes << "Waypoints";
        if (engagementsCheckBox->isChecked()) eventTypes << "Engagements";
        emit eventTypesSelected(eventTypes);
    });
}


#include "timingdialog.h"
#include <QJsonParseError>
#include <QTime>
#include <QDebug>
#include <QScrollBar>
#include <QPushButton>
#include <QGroupBox>
#include "GUI/Editors/runtimeeditor.h"

TimingWidget::TimingWidget(QWidget *parent) : QWidget(parent)
{
    timeUnit = "Minutes";
    setMinimumWidth(700);
}

void TimingWidget::setTimingData(const QList<QMap<QString, QString>> &data)
{
    timingData = data;
    int height = 50 + 60 * data.size();
    setMinimumHeight(height);
    update();
    qDebug() << "TimingWidget: Set data for" << data.size() << "entities, height:" << height;
}

void TimingWidget::setTimeUnit(const QString &unit)
{
    timeUnit = unit;
    update();
    qDebug() << "TimingWidget: Time unit changed to" << unit;
}

void TimingWidget::setTeamDivision(const QMap<QString, QString> &teamMap)
{
    this->teamMap = teamMap;
    update();
}

QString TimingWidget::calculateTotalTime(const QString &startTime, const QString &endTime, const QString &unit)
{
    QTime start = QTime::fromString(startTime, "hh:mm:ss");
    QTime end = QTime::fromString(endTime, "hh:mm:ss");
    if (!start.isValid() || !end.isValid() || endTime.isEmpty()) {
        return "Incomplete";
    }
    int seconds = start.secsTo(end);
    if (unit == "Hours") {
        double hours = seconds / 3600.0;
        return QString::number(hours, 'f', 2) + " hrs";
    } else if (unit == "Minutes") {
        double minutes = seconds / 60.0;
        return QString::number(minutes, 'f', 2) + " min";
    } else if (unit == "Seconds") {
        return QString::number(seconds) + " sec";
    }
    return "Invalid";
}

QList<QMap<QString, QString>> TimingWidget::getIncompleteEntities()
{
    QList<QMap<QString, QString>> incomplete;
    for (const auto &data : timingData) {
        if (data["endTime"].isEmpty()) {
            incomplete.append(data);
        }
    }
    return incomplete;
}

QMap<QString, QString> TimingWidget::getTeamDivision() const
{
    return teamMap;
}

void TimingWidget::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    int width = this->width();
    if (width <= 0) return;
    int entityColX = 10;
    int startTimeColX = 80;
    int timelineStartX = 180;
    int timelineEndX = width - 250;
    int endTimeColX = timelineEndX + 10;
    int totalTimeColX = endTimeColX + 90;
    int statusColX = totalTimeColX + 100;
    if (statusColX + 80 > width) {
        timelineEndX = width - 330;
        endTimeColX = timelineEndX + 10;
        totalTimeColX = endTimeColX + 90;
        statusColX = totalTimeColX + 100;
        if (statusColX + 80 > width) {
            timelineEndX = width - 350;
            endTimeColX = timelineEndX + 10;
            totalTimeColX = endTimeColX + 80;
            statusColX = totalTimeColX + 80;
        }
    }
    int lineHeight = 60;
    int headerY = 10;
    int firstEntityY = 40;
    painter.setPen(Qt::black);
    painter.setFont(QFont("Arial", 10, QFont::Bold));
    painter.drawText(QRect(entityColX, headerY, 60, 20), Qt::AlignLeft, "Entity");
    painter.drawText(QRect(startTimeColX, headerY, 90, 20), Qt::AlignLeft, "Start Time");
    painter.drawText(QRect(endTimeColX, headerY, 90, 20), Qt::AlignLeft, "End Time");
    painter.drawText(QRect(totalTimeColX, headerY, 90, 20), Qt::AlignLeft, "Total Time");
    painter.drawText(QRect(statusColX, headerY, 80, 20), Qt::AlignLeft, "Status");
    painter.setFont(QFont("Arial", 9));
    for (int i = 0; i < timingData.size(); ++i) {
        const QMap<QString, QString> &data = timingData[i];
        QString entity = data["entity"];
        QString startTime = data["startTime"];
        QString endTime = data["endTime"];
        QString activeStatus = data.value("active", "true");
        QString totalTime = calculateTotalTime(startTime, endTime, timeUnit);
        int y = firstEntityY + i * lineHeight;
        QColor teamColor = Qt::gray;
        if (teamMap.contains(entity)) {
            if (teamMap[entity] == "blue") teamColor = Qt::blue;
            else if (teamMap[entity] == "red") teamColor = Qt::red;
        }
        painter.setPen(Qt::black);
        painter.setBrush(teamColor);
        painter.drawRect(entityColX, y - 15, 60, 20);
        painter.setPen(Qt::white);
        painter.drawText(QRect(entityColX, y - 15, 60, 20), Qt::AlignCenter, entity);
        painter.setPen(Qt::black);
        painter.setBrush(Qt::NoBrush);
        painter.drawText(QRect(startTimeColX, y - 15, 90, 20), Qt::AlignLeft, startTime);
        if (endTime.isEmpty()) {
            painter.setPen(QColorConstants::Svg::pink);
            painter.drawText(QRect(endTimeColX, y - 15, 90, 20), Qt::AlignLeft, "N/A");
        } else {
            painter.setPen(Qt::black);
            painter.drawText(QRect(endTimeColX, y - 15, 90, 20), Qt::AlignLeft, endTime);
        }
        if (endTime.isEmpty()) {
            painter.setPen(QPen(QColorConstants::Svg::pink, 2, Qt::DashLine));
            painter.drawLine(timelineStartX, y, timelineEndX, y);
            painter.setBrush(QColorConstants::Svg::pink);
            painter.drawEllipse(timelineStartX - 5, y - 5, 10, 10);
        } else {
            painter.setPen(QPen(teamColor, 2));
            painter.drawLine(timelineStartX, y, timelineEndX, y);
            painter.setBrush(teamColor);
            painter.drawEllipse(timelineStartX - 5, y - 5, 10, 10);
            painter.drawEllipse(timelineEndX - 5, y - 5, 10, 10);
        }
        painter.setPen(endTime.isEmpty() ? QColorConstants::Svg::pink : Qt::black);
        painter.drawText(QRect(totalTimeColX, y - 15, 90, 20), Qt::AlignLeft, totalTime);
        qDebug() << "Entity:" << entity << "Active status:" << activeStatus;
        if (activeStatus.compare("true", Qt::CaseInsensitive) == 0) {
            painter.setPen(Qt::darkGreen);
            painter.drawText(QRect(statusColX, y - 15, 80, 20), Qt::AlignLeft, "Active");
        } else {
            painter.setPen(Qt::red);
            painter.drawText(QRect(statusColX, y - 15, 80, 20), Qt::AlignLeft, "Inactive");
        }
    }
}

IncompleteEntitiesWidget::IncompleteEntitiesWidget(QWidget *parent) : QWidget(parent)
{
    layout = new QVBoxLayout(this);
    setLayout(layout);
    QLabel *titleLabel = new QLabel("Incomplete Entities", this);
    titleLabel->setStyleSheet("QLabel { font-weight: bold; color: pink; }");
    layout->addWidget(titleLabel);
    QFrame *line = new QFrame(this);
    line->setFrameShape(QFrame::HLine);
    line->setFrameShadow(QFrame::Sunken);
    layout->addWidget(line);
    setMinimumWidth(200);
    setMaximumWidth(250);
}

void IncompleteEntitiesWidget::setIncompleteEntities(const QList<QMap<QString, QString>> &entities)
{
    for (int i = layout->count() - 1; i >= 2; i--) {
        QLayoutItem *item = layout->itemAt(i);
        if (item->widget()) {
            item->widget()->deleteLater();
        }
        layout->removeItem(item);
    }
    if (entities.isEmpty()) {
        QLabel *noEntitiesLabel = new QLabel("No incomplete entities", this);
        noEntitiesLabel->setStyleSheet("QLabel { color: green; }");
        layout->addWidget(noEntitiesLabel);
        return;
    }
    for (const auto &entity : entities) {
        QString info = QString("%1\nStart: %2")
                           .arg(entity["entity"])
                           .arg(entity["startTime"]);
        QLabel *entityLabel = new QLabel(info, this);
        entityLabel->setStyleSheet("QLabel { background-color: #ffffee; padding: 5px; border: 1px solid #ffffcc; }");
        entityLabel->setWordWrap(true);
        layout->addWidget(entityLabel);
    }
}

TimingDialog::TimingDialog(QWidget *parent) : QMainWindow(parent), selectedTeam("blue")
{
    setStyleSheet("QMainWindow { border: 2px solid black; }");
    setWindowTitle(tr("Entity Timing Information"));
    setMinimumSize(1200, 800);
    QWidget *centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);
    QHBoxLayout *mainLayout = new QHBoxLayout(centralWidget);
    QVBoxLayout *leftLayout = new QVBoxLayout();
    QLabel *entityListLabel = new QLabel("Entities", this);
    entityListLabel->setStyleSheet("QLabel { font-weight: bold; }");
    leftLayout->addWidget(entityListLabel);
    entityListWidget = new QListWidget(this);
    entityListWidget->setMinimumWidth(150);
    entityListWidget->setMaximumWidth(150);
    leftLayout->addWidget(entityListWidget);
    leftLayout->addSpacing(20);
    teamAButton = new QPushButton("Select Team A (Blue)", this);
    teamAButton->setStyleSheet("QPushButton { background-color: blue; color: white; }");
    leftLayout->addWidget(teamAButton);
    teamBButton = new QPushButton("Select Team B (Red)", this);
    teamBButton->setStyleSheet("QPushButton { background-color: red; color: white; }");
    leftLayout->addWidget(teamBButton);
    bothTeamsButton = new QPushButton("Select Both Teams", this);
    bothTeamsButton->setStyleSheet("QPushButton { background-color: purple; color: white; }");
    leftLayout->addWidget(bothTeamsButton);
    QPushButton *runSimulationButton = new QPushButton("Run Simulation", this);
    runSimulationButton->setStyleSheet("QPushButton { background-color: green; color: white; }");
    leftLayout->addWidget(runSimulationButton);
    leftLayout->addStretch();
    QVBoxLayout *middleLayout = new QVBoxLayout();
    QHBoxLayout *controlsLayout = new QHBoxLayout();
    QLabel *timeUnitLabel = new QLabel("Time Unit:", this);
    controlsLayout->addWidget(timeUnitLabel);
    timeUnitComboBox = new QComboBox(this);
    timeUnitComboBox->addItems({"Hours", "Minutes", "Seconds"});
    timeUnitComboBox->setCurrentText("Minutes");
    timeUnitComboBox->setMinimumWidth(100);
    controlsLayout->addWidget(timeUnitComboBox);
    controlsLayout->addStretch();
    middleLayout->addLayout(controlsLayout);
    this->timingWidget = new TimingWidget(this);
    QScrollArea *scrollArea = new QScrollArea(this);
    scrollArea->setWidget(this->timingWidget);
    scrollArea->setWidgetResizable(true);
    scrollArea->setMinimumHeight(300);
    scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    middleLayout->addWidget(scrollArea);
    this->graphWidgetTime = new GraphWidgetTime(this);
    middleLayout->addWidget(graphWidgetTime);
    QVBoxLayout *rightLayout = new QVBoxLayout();
    this->incompleteWidget = new IncompleteEntitiesWidget(this);
    rightLayout->addWidget(incompleteWidget);
    rightLayout->addStretch();
    mainLayout->addLayout(leftLayout);
    mainLayout->addLayout(middleLayout, 2);
    mainLayout->addLayout(rightLayout);
    centralWidget->setLayout(mainLayout);
    setupTeams();
    QString jsonData;
    RuntimeEditor *runtimeEditor = qobject_cast<RuntimeEditor *>(parent);
    if (runtimeEditor) {
        jsonData = runtimeEditor->getTimingJsonData();
    } else {
        jsonData = GraphWidgetTime::JSON_DATA;
    }
    populateEntityList(jsonData);
    connect(teamAButton, &QPushButton::clicked, this, [=]() { onTeamSelected("blue"); });
    connect(teamBButton, &QPushButton::clicked, this, [=]() { onTeamSelected("red"); });
    connect(bothTeamsButton, &QPushButton::clicked, this, [=]() { onTeamSelected("both"); });
    connect(timeUnitComboBox, &QComboBox::currentTextChanged, this, [=](const QString &unit) {
        if (this->timingWidget) {
            this->timingWidget->setTimeUnit(unit);
            QList<QMap<QString, QString>> incomplete = this->timingWidget->getIncompleteEntities();
            this->incompleteWidget->setIncompleteEntities(incomplete);
        }
    });
    connect(runSimulationButton, &QPushButton::clicked, this, [=]() { graphWidgetTime->startSimulation(); });
    qDebug() << "TimingDialog: Constructor completed";
}

void TimingDialog::setupTeams()
{
}

void TimingDialog::startGraphSimulation()
{
    graphWidgetTime->startSimulation();
}

void TimingDialog::onTeamSelected(const QString &team)
{
    selectedTeam = team;
    qDebug() << "TimingDialog: Selected team:" << team;
    if (this->graphWidgetTime) {
        QJsonParseError parseError;
        QJsonDocument doc = QJsonDocument::fromJson(GraphWidgetTime::JSON_DATA.toUtf8(), &parseError);
        if (parseError.error == QJsonParseError::NoError && doc.isArray()) {
            QJsonArray jsonArray = doc.array();
            QList<QMap<QString, QString>> timingDataList;
            for (const QJsonValue &value : jsonArray) {
                QJsonObject obj = value.toObject();
                QMap<QString, QString> data;
                data["entity"] = obj["entity"].toString();
                data["startTime"] = obj["startTime"].toString();
                data["endTime"] = obj["endTime"].toString();
                data["active"] = obj["active"].toString();
                timingDataList.append(data);
            }
            this->graphWidgetTime->setTimingData(timingDataList, selectedTeam);
        }
    }
}

void TimingDialog::populateEntityList(const QString &jsonData)
{
    qDebug() << "TimingDialog: Parsing JSON";
    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(jsonData.toUtf8(), &parseError);
    if (parseError.error != QJsonParseError::NoError) {
        qDebug() << "TimingDialog: JSON parse error:" << parseError.errorString();
        QListWidgetItem *item = new QListWidgetItem("Error parsing JSON");
        entityListWidget->addItem(item);
        return;
    }
    if (!doc.isArray()) {
        qDebug() << "TimingDialog: JSON is not an array";
        QListWidgetItem *item = new QListWidgetItem("Invalid JSON format");
        entityListWidget->addItem(item);
        return;
    }
    QJsonArray jsonArray = doc.array();
    QList<QMap<QString, QString>> timingDataList;
    entityListWidget->clear();
    teamMap.clear();
    for (const QJsonValue &value : jsonArray) {
        QJsonObject obj = value.toObject();
        QString entity = obj["entity"].toString();
        QListWidgetItem *item = new QListWidgetItem(entity);
        entityListWidget->addItem(item);
        QMap<QString, QString> data;
        data["entity"] = obj["entity"].toString();
        data["startTime"] = obj["startTime"].toString();
        data["endTime"] = obj["endTime"].toString();
        data["active"] = obj["active"].toString();
        if (obj.contains("team")) {
            QString team = obj["team"].toString().toLower();
            teamMap[entity] = team;
        }
        qDebug() << "Parsed entity:" << entity << "active:" << data["active"] << "team:" << (obj.contains("team") ? obj["team"].toString() : "not specified");
        timingDataList.append(data);
    }
    qDebug() << "TimingDialog: Populated" << timingDataList.size() << "entities";
    if (this->timingWidget) {
        this->timingWidget->setTimingData(timingDataList);
        this->timingWidget->setTeamDivision(teamMap);
        QList<QMap<QString, QString>> incomplete = this->timingWidget->getIncompleteEntities();
        this->incompleteWidget->setIncompleteEntities(incomplete);
        this->graphWidgetTime->setTeamDivision(teamMap);
        this->graphWidgetTime->setTimingData(timingDataList, selectedTeam);
        this->graphWidgetTime->update();
        QScrollArea *scrollArea = qobject_cast<QScrollArea *>(this->timingWidget->parentWidget());
        if (scrollArea) {
            if (timingDataList.size() > 5) {
                scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
            } else {
                scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
            }
        } else {
            qDebug() << "TimingDialog: Failed to find QScrollArea";
        }
    } else {
        qDebug() << "TimingDialog: timingWidget is null";
    }
}

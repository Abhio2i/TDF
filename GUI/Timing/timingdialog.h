
#ifndef TIMINGDIALOG_H
#define TIMINGDIALOG_H

#include <QMainWindow>
#include <QListWidget>
#include <QWidget>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QComboBox>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QPainter>
#include <QScrollArea>
#include <QLabel>
#include <QFrame>
#include <QMap>
#include <QSet>
#include <QPushButton>
#include "graphwidgettime.h"

class TimingWidget : public QWidget
{
    Q_OBJECT
public:
    explicit TimingWidget(QWidget *parent = nullptr);
    void setTimingData(const QList<QMap<QString, QString>> &data);
    void setTimeUnit(const QString &unit);
    void setTeamDivision(const QMap<QString, QString> &teamMap);
    QString calculateTotalTime(const QString &startTime, const QString &endTime, const QString &unit = "Minutes");
    QList<QMap<QString, QString>> getIncompleteEntities();
    QMap<QString, QString> getTeamDivision() const;
protected:
    void paintEvent(QPaintEvent *event) override;
private:
    QList<QMap<QString, QString>> timingData;
    QString timeUnit;
    QMap<QString, QString> teamMap;
};

class IncompleteEntitiesWidget : public QWidget
{
    Q_OBJECT
public:
    explicit IncompleteEntitiesWidget(QWidget *parent = nullptr);
    void setIncompleteEntities(const QList<QMap<QString, QString>> &entities);
private:
    QVBoxLayout *layout;
};

class TimingDialog : public QMainWindow
{
    Q_OBJECT
public:
    explicit TimingDialog(QWidget *parent = nullptr);
public slots:
    void startGraphSimulation(); // Added new slot
private slots:
    void onTeamSelected(const QString &team);
private:
    QListWidget *entityListWidget;
    TimingWidget *timingWidget;
    GraphWidgetTime *graphWidgetTime;
    IncompleteEntitiesWidget *incompleteWidget;
    QComboBox *timeUnitComboBox;
    QPushButton *teamAButton;
    QPushButton *teamBButton;
    QPushButton *bothTeamsButton;
    QMap<QString, QString> teamMap;
    QString selectedTeam;
    void populateEntityList(const QString &jsonData);
    void setupTeams();
};

#endif // TIMINGDIALOG_H

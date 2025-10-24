/* ========================================================================= */
/* File: timingdialog.h                                                     */
/* Purpose: Defines widgets for timing data visualization and management      */
/* ========================================================================= */

#ifndef TIMINGDIALOG_H
#define TIMINGDIALOG_H

#include <QMainWindow>                            // For main window base class
#include <QListWidget>                            // For list widget
#include <QWidget>                                // For widget base class
#include <QHBoxLayout>                            // For horizontal layout
#include <QVBoxLayout>                            // For vertical layout
#include <QComboBox>                              // For combo box widget
#include <QJsonDocument>                          // For JSON document handling
#include <QJsonArray>                             // For JSON array handling
#include <QJsonObject>                            // For JSON object handling
#include <QPainter>                               // For painting operations
#include <QScrollArea>                            // For scroll area widget
#include <QLabel>                                 // For label widget
#include <QFrame>                                 // For frame widget
#include <QMap>                                   // For map container
#include <QSet>                                   // For set container
#include <QPushButton>                            // For push button widget
#include "graphwidgettime.h"                      // For time graph widget

// %%% TimingWidget Class %%%
/* Widget for displaying timing data */
class TimingWidget : public QWidget
{
    Q_OBJECT

public:
    // Initialize timing widget
    explicit TimingWidget(QWidget *parent = nullptr);
    // Set timing data
    void setTimingData(const QList<QMap<QString, QString>> &data);
    // Set time unit
    void setTimeUnit(const QString &unit);
    // Set team division mapping
    void setTeamDivision(const QMap<QString, QString> &teamMap);
    // Calculate total time
    QString calculateTotalTime(const QString &startTime, const QString &endTime, const QString &unit = "Minutes");
    // Get incomplete entities
    QList<QMap<QString, QString>> getIncompleteEntities();
    // Get team division map
    QMap<QString, QString> getTeamDivision() const;

protected:
    // Handle paint events
    void paintEvent(QPaintEvent *event) override;

private:
    // %%% Data Members %%%
    // Timing data list
    QList<QMap<QString, QString>> timingData;
    // Time unit
    QString timeUnit;
    // Team division map
    QMap<QString, QString> teamMap;
};

// %%% IncompleteEntitiesWidget Class %%%
/* Widget for displaying incomplete entities */
class IncompleteEntitiesWidget : public QWidget
{
    Q_OBJECT

public:
    // Initialize incomplete entities widget
    explicit IncompleteEntitiesWidget(QWidget *parent = nullptr);
    // Set incomplete entities data
    void setIncompleteEntities(const QList<QMap<QString, QString>> &entities);

private:
    // %%% UI Components %%%
    // Layout for widget
    QVBoxLayout *layout;
};

// %%% TimingDialog Class %%%
/* Main window for timing data management */
class TimingDialog : public QMainWindow
{
    Q_OBJECT

public:
    // Initialize timing dialog
    explicit TimingDialog(QWidget *parent = nullptr);

public slots:
    // Start graph simulation
    void startGraphSimulation();

private slots:
    // Handle team selection
    void onTeamSelected(const QString &team);

private:
    // %%% UI Components %%%
    // Entity list widget
    QListWidget *entityListWidget;
    // Timing widget
    TimingWidget *timingWidget;
    // Time graph widget
    GraphWidgetTime *graphWidgetTime;
    // Incomplete entities widget
    IncompleteEntitiesWidget *incompleteWidget;
    // Time unit combo box
    QComboBox *timeUnitComboBox;
    // Team A button
    QPushButton *teamAButton;
    // Team B button
    QPushButton *teamBButton;
    // Both teams button
    QPushButton *bothTeamsButton;
    // Team division map
    QMap<QString, QString> teamMap;
    // Selected team name
    QString selectedTeam;

    // %%% Utility Methods %%%
    // Populate entity list from JSON
    void populateEntityList(const QString &jsonData);
    // Setup team buttons
    void setupTeams();
};

#endif // TIMINGDIALOG_H

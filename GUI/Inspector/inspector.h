

#ifndef INSPECTOR_H
#define INSPECTOR_H

#include <QDockWidget>
#include <QTableWidget>
#include <QJsonObject>
#include <QLabel>
#include <QLineEdit>
#include <QWheelEvent>
#include <QPushButton>
#include <QMenu>
#include <QListWidget>
#include <QJsonArray>
#include "core/Hierarchy/hierarchy.h"

// Forward declarations for template classes
class ColorTemplate;
class ImageTemplate;
class GeocordsTemplate;
class OptionTemplate;
class VectorTemplate;
class CustomParameterDialog;

class WheelableLineEdit : public QLineEdit {
    Q_OBJECT
public:
    explicit WheelableLineEdit(QWidget *parent = nullptr);
protected:
    void wheelEvent(QWheelEvent *event) override;
};

class Inspector : public QDockWidget
{
    Q_OBJECT

public:
    explicit Inspector(QWidget *parent = nullptr);
    bool eventFilter(QObject *watched, QEvent *event) override;
    void setHierarchy(Hierarchy* h) { hierarchy = h; }
    static QString formatNumberForUI(double value);


public slots:
    void init(QString ID, QString name, QJsonObject obj);
    int addSimpleRow(int row, const QString &key, const QJsonValue &value);
    void setupValueCell(int row, const QString &fullKey, const QJsonValue &value);
    void updateTrajectory(QString entityId, QJsonArray waypoints);

signals:
    void foucsEntity(QString ID); // Note: Typo "foucsEntity" should be "focusEntity"
    void valueChanged(QString ID, QString name, QJsonObject delta);
    void addTabRequested();
    void parameterChanged(QString ID, QString name, QString key, QString parameterType, bool add);
    void trajectoryWaypointsChanged(QString entityId, QJsonArray waypoints);

private slots:
    void copyCurrentComponent();
    void pasteToCurrentComponent();
    void handleAddTab();
    void handleAddParameter();
    void handleRemoveParameter();

private:
    QTableWidget *tableWidget;
    QLabel *titleLabel;
    QString ConnectedID;
    QString Name;
    QMap<int, QString> rowToKeyPath;
    QSet<QString> customParameterKeys;
    QWidget *titleBarWidget;
    QPushButton *menuButton;
    QJsonObject copiedComponentData;
    QString copiedComponentType;
    Hierarchy* hierarchy = nullptr;
    QJsonObject copiedVectorData;

    void setupUI();
    void setupTitleBar();
    QMenu *createContextMenu();
    void setupBooleanCell(int row, const QString &fullKey, bool value);
    void setupArrayCell(int row, const QString &fullKey, const QJsonArray &array);
    void setupStringCell(int row, const QString &fullKey, const QString &value);
    void setupNumberCell(int row, const QString &fullKey, double value);
    void setupGenericObjectCell(int row, const QString &fullKey, const QJsonObject &obj);
    void addParameterRow(const QString &parameterName, int row);
    QPushButton *createRemoveButton(const QString &parameterName);
};
#endif

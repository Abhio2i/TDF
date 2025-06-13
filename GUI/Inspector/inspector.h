
#ifndef INSPECTOR_H
#define INSPECTOR_H

/* Includes section */
#include <QDockWidget>
#include <QTableWidget>
#include <QJsonObject>
#include <QLabel>
#include <QLineEdit>
#include <QWheelEvent>
#include <QPushButton>
#include <QMenu>

#include <QListWidget>
#include "core/Hierarchy/hierarchy.h"
class CustomParameterDialog;

/* WheelableLineEdit class section */
class WheelableLineEdit : public QLineEdit {
    Q_OBJECT
public:
    explicit WheelableLineEdit(QWidget *parent = nullptr);
protected:
    void wheelEvent(QWheelEvent *event) override;
};

/* Inspector class section */
class Inspector : public QDockWidget
{
    Q_OBJECT

public:
    explicit Inspector(QWidget *parent = nullptr);
    bool eventFilter(QObject *watched, QEvent *event) override;
    void setHierarchy(Hierarchy* h) { hierarchy = h; }

public slots:
    void init(QString ID, QString name, QJsonObject obj);
    int addSimpleRow(int row, const QString &key, const QJsonValue &value);
    void setupValueCell(int row, const QString &fullKey, const QJsonValue &value);

signals:
    void foucsEntity(QString ID);
    void valueChanged(QString ID, QString name, QJsonObject delta);
    void addTabRequested();
    void parameterChanged(QString ID, QString name, QString key, QString parameterType, bool add); // Added signal

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

    void setupUI();
    void setupTitleBar();
    QMenu *createContextMenu();

    QJsonObject currentVectorData;
    QString currentVectorKey;
    QJsonObject copiedVectorData;
};

#endif // INSPECTOR_H

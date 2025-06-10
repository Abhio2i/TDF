
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
#include "core/structure/hierarchy.h"
#include <QListWidget>

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

public slots:
    void init(QString ID, QString name, QJsonObject obj);
    int addSimpleRow(int row, const QString &key, const QJsonValue &value);
    void setupValueCell(int row, const QString &fullKey, const QJsonValue &value);

signals:
    void foucsEntity(QString ID);
    void valueChanged(QString ID, QString name, QJsonObject delta);
    void addTabRequested();

private slots:
    void copyCurrentComponent();
    void pasteToCurrentComponent();
    void handleAddTab();

private:
    QTableWidget *tableWidget;
    QLabel *titleLabel;
    QString ConnectedID;
    QString Name;
    QMap<int, QString> rowToKeyPath;
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



#ifndef VECTORTEMPLATE_H
#define VECTORTEMPLATE_H

#include <QWidget>
#include <QJsonObject>
#include <QTableWidget>
#include "GUI/Inspector/inspector.h"

class VectorTemplate : public QWidget
{
    Q_OBJECT
public:
    explicit VectorTemplate(QWidget *parent = nullptr);
    void setupVectorCell(int row, const QString &fullKey, const QJsonObject &obj, QTableWidget *tableWidget);
    void setConnectedID(const QString &id) { connectedID = id; }
    void setName(const QString &n) { name = n; }
    static constexpr int ROW_HEIGHT = 30;


signals:
    void valueChanged(QString ID, QString name, QJsonObject delta);

private:
    QString connectedID;
    QString name;
    QJsonObject copiedVectorData;
};

#endif

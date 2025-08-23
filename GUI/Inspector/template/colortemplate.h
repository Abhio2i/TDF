#ifndef COLORTEMPLATE_H
#define COLORTEMPLATE_H

#include <QWidget>
#include <QJsonObject>
#include <QPushButton>
#include <QTableWidget>

class ColorTemplate : public QWidget
{
    Q_OBJECT
public:
    explicit ColorTemplate(QWidget *parent = nullptr);
    void setupColorCell(int row, const QString &fullKey, const QJsonObject &obj, QTableWidget *tableWidget);
    void setConnectedID(const QString &id) { connectedID = id; }
    void setName(const QString &n) { name = n; }

    static constexpr int ROW_HEIGHT = 30;

signals:
    void valueChanged(QString ID, QString name, QJsonObject delta);

private:
    QString connectedID;
    QString name;
};

#endif

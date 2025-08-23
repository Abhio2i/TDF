

#ifndef IMAGETEMPLATE_H
#define IMAGETEMPLATE_H

#include <QWidget>
#include <QJsonObject>
#include <QTableWidget>

class ImageTemplate : public QWidget
{
    Q_OBJECT
public:
    explicit ImageTemplate(QWidget *parent = nullptr);
    void setupImageCell(int row, const QString &fullKey, const QJsonObject &obj, QTableWidget *tableWidget);
    void setConnectedID(const QString &id) { connectedID = id; }
    void setName(const QString &n) { name = n; }

    static constexpr int ROW_HEIGHT = 100;
    static constexpr int IMAGE_SIZE = 60;

signals:
    void valueChanged(QString ID, QString name, QJsonObject delta);

private:
    QString connectedID;
    QString name;
};

#endif

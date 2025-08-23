#ifndef OPTIONTEMPLATE_H
#define OPTIONTEMPLATE_H

#include <QWidget>
#include <QJsonObject>
#include <QJsonArray>
#include <QTableWidget>
#include <QComboBox>

class OptionTemplate : public QWidget
{
    Q_OBJECT
public:
    explicit OptionTemplate(QWidget *parent = nullptr);
    void setupOptionCell(int row, const QString &fullKey, const QJsonObject &obj, QTableWidget *tableWidget);
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



#ifndef GISNETWORK_H
#define GISNETWORK_H

#include <QNetworkAccessManager>
#include <QWidget>

class GISNetwork : public QWidget {
    Q_OBJECT
public:
    explicit GISNetwork(QWidget *parent = nullptr);
    void requestImage(QUrl url);
    void requestPlace(const QString& query);

signals:
    void receiveImage(QString url,QByteArray data);
    void receivePlace(QString url,QByteArray data);


private:
    QNetworkAccessManager manager;
};

#endif // GISNETWORK_H

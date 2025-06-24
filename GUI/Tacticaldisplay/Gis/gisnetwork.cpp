

// #include "gisnetwork.h"
// #include "qnetworkreply.h"
// #include "qurlquery.h"

// GISNetwork::GISNetwork(QWidget *parent)
//     : QWidget(parent)
// {
//     // Manager ready to use
// }

// void GISNetwork::requestImage(QUrl url){
//     QNetworkRequest request(url);
//     request.setHeader(QNetworkRequest::UserAgentHeader, "MyQtApp/1.0 (Contact: your-email@example.com)");
//     QNetworkReply* reply = manager.get(request);

//     connect(reply, &QNetworkReply::finished, this, [=]() {
//         QByteArray data = reply->readAll();
//         qDebug() << "Downloaded size:" << data.size();
//         emit receiveImage(reply->url().toString(),data);
//         reply->deleteLater();
//     });

// }

// void GISNetwork::requestPlace(const QString& query) {
//     QString baseUrl = "https://nominatim.openstreetmap.org/search";
//     QUrl url(baseUrl);
//     QUrlQuery queryParams;
//     queryParams.addQueryItem("q", query);
//     queryParams.addQueryItem("format", "json");
//     queryParams.addQueryItem("polygon_geojson", "1");

//     url.setQuery(queryParams);

//     QNetworkRequest request(url);
//     request.setHeader(QNetworkRequest::UserAgentHeader, "GISViewerApp/1.0 (contact: pankaj@example.com)");

//     QNetworkReply* reply = manager.get(request);
//     connect(reply, &QNetworkReply::finished, this, [=]() {
//         QByteArray data = reply->readAll();
//         qDebug() << "Downloaded size:" << data.size();
//         //qDebug() << "Raw JSON:" << QString::fromUtf8(data);
//         emit receivePlace(reply->url().toString(), data);
//         reply->deleteLater();
//     });
// }

#include "gisnetwork.h"
#include "qnetworkreply.h"
#include "qurlquery.h"

GISNetwork::GISNetwork(QWidget *parent)
    : QWidget(parent)
{
    // Manager ready to use
}

void GISNetwork::requestImage(QUrl url){
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::UserAgentHeader, "MyQtApp/1.0 (Contact: your-email@example.com)");
    QNetworkReply* reply = manager.get(request);

    connect(reply, &QNetworkReply::finished, this, [=]() {
        QByteArray data = reply->readAll();
        qDebug() << "Downloaded size:" << data.size();
        emit receiveImage(reply->url().toString(),data);
        reply->deleteLater();
    });

}

void GISNetwork::requestPlace(const QString& query) {
    QString baseUrl = "https://nominatim.openstreetmap.org/search";
    QUrl url(baseUrl);
    QUrlQuery queryParams;
    queryParams.addQueryItem("q", query);
    queryParams.addQueryItem("format", "json");
    queryParams.addQueryItem("polygon_geojson", "1");

    url.setQuery(queryParams);

    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::UserAgentHeader, "GISViewerApp/1.0 (contact: pankaj@example.com)");

    QNetworkReply* reply = manager.get(request);
    connect(reply, &QNetworkReply::finished, this, [=]() {
        QByteArray data = reply->readAll();
        qDebug() << "Downloaded size:" << data.size();
        //qDebug() << "Raw JSON:" << QString::fromUtf8(data);
        emit receivePlace(reply->url().toString(), data);
        reply->deleteLater();
    });
}

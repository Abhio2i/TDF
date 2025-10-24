

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


/*
 * GISNetwork Implementation File
 * This file contains the implementation of the GISNetwork class which provides
 * network functionality for downloading map tiles and performing geographic
 * place searches through HTTP requests.
 */

#include "gisnetwork.h"
#include "qnetworkreply.h"
#include "qurlquery.h"

/*
 * Constructor: GISNetwork
 * Initializes the network access manager for handling HTTP requests
 * The manager is ready to use immediately after construction
 */
GISNetwork::GISNetwork(QWidget *parent)
    : QWidget(parent)
{
    // QNetworkAccessManager is automatically initialized and ready to use
    // No additional setup required as Qt handles the internal initialization
}

/*
 * requestImage: Download a map tile image from specified URL
 * Asynchronously fetches tile images from tile servers and emits receiveImage signal on completion
 */
void GISNetwork::requestImage(QUrl url){
    // Create network request with proper URL
    QNetworkRequest request(url);

    // Set user agent header to identify the application to tile servers
    // This is required by many tile servers for usage tracking and fair use policies
    request.setHeader(QNetworkRequest::UserAgentHeader, "MyQtApp/1.0 (Contact: your-email@example.com)");

    // Initiate asynchronous GET request
    QNetworkReply* reply = manager.get(request);

    // Connect finished signal to handle response
    connect(reply, &QNetworkReply::finished, this, [=]() {
        // Read all response data into byte array
        QByteArray data = reply->readAll();

        // Debug output to monitor download sizes
        qDebug() << "Downloaded size:" << data.size();

        // Emit signal with URL and image data for processing
        emit receiveImage(reply->url().toString(), data);

        // Clean up the reply object to prevent memory leaks
        reply->deleteLater();
    });
}

/*
 * requestPlace: Search for geographic places using OpenStreetMap Nominatim API
 * Performs forward geocoding to convert place names to geographic coordinates
 */
void GISNetwork::requestPlace(const QString& query) {
    // Base URL for OpenStreetMap Nominatim search API
    QString baseUrl = "https://nominatim.openstreetmap.org/search";
    QUrl url(baseUrl);

    // Build query parameters for the search request
    QUrlQuery queryParams;
    queryParams.addQueryItem("q", query);              // Search query string
    queryParams.addQueryItem("format", "json");        // Response format
    queryParams.addQueryItem("polygon_geojson", "1");  // Include geographic outlines

    // Set the complete URL with query parameters
    url.setQuery(queryParams);

    // Create network request with proper headers
    QNetworkRequest request(url);

    // Set user agent as required by Nominatim usage policy
    // Important: Replace with your actual application name and contact information
    request.setHeader(QNetworkRequest::UserAgentHeader, "GISViewerApp/1.0 (contact: pankaj@example.com)");

    // Initiate asynchronous GET request
    QNetworkReply* reply = manager.get(request);

    // Connect finished signal to handle response
    connect(reply, &QNetworkReply::finished, this, [=]() {
        // Read all response data
        QByteArray data = reply->readAll();

        // Debug output for monitoring
        qDebug() << "Downloaded size:" << data.size();

        // Uncomment for detailed JSON debugging:
        // qDebug() << "Raw JSON:" << QString::fromUtf8(data);

        // Emit signal with URL and search results for processing
        emit receivePlace(reply->url().toString(), data);

        // Clean up the reply object
        reply->deleteLater();
    });
}



// #ifndef GISNETWORK_H
// #define GISNETWORK_H

// #include <QNetworkAccessManager>
// #include <QWidget>

// class GISNetwork : public QWidget {
//     Q_OBJECT
// public:
//     explicit GISNetwork(QWidget *parent = nullptr);
//     void requestImage(QUrl url);
//     void requestPlace(const QString& query);

// signals:
//     void receiveImage(QString url,QByteArray data);
//     void receivePlace(QString url,QByteArray data);


// private:
//     QNetworkAccessManager manager;
// };

// #endif // GISNETWORK_H


/*
 * GISNetwork Header File
 * This file defines the GISNetwork class which provides network functionality
 * for downloading map tiles and performing geographic place searches using
 * the OpenStreetMap Nominatim API and various tile servers.
 */

#ifndef GISNETWORK_H
#define GISNETWORK_H

// Include necessary Qt network classes
#include <QNetworkAccessManager>
#include <QWidget>

/*
 * GISNetwork Class
 * A QWidget-based network manager that handles HTTP requests for map tiles
 * and geographic place searches. Provides asynchronous downloading with
 * signal-based completion notifications.
 */
class GISNetwork : public QWidget {
    Q_OBJECT  // Qt macro for signals/slots support

public:
    /*
     * Constructor
     * Initializes the network manager for handling HTTP requests
     */
    explicit GISNetwork(QWidget *parent = nullptr);

    /*
     * Public Methods for Network Requests
     */

    /* Request a map tile image from the specified URL */
    void requestImage(QUrl url);

    /* Search for a geographic place using the Nominatim API */
    void requestPlace(const QString& query);

signals:
    /*
     * Network Response Signals
     * Emitted when network requests complete with their results
     */

    /* Signal emitted when a tile image download completes */
    void receiveImage(QString url, QByteArray data);

    /* Signal emitted when a place search request completes */
    void receivePlace(QString url, QByteArray data);

private:
    /*
     * Network Manager Instance
     * Handles all HTTP requests and network operations
     */
    QNetworkAccessManager manager;
};

#endif // GISNETWORK_H

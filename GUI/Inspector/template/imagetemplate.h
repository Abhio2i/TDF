/* ========================================================================= */
/* File: imagetemplate.h                                                    */
/* Purpose: Defines widget for managing image templates                      */
/* ========================================================================= */

#ifndef IMAGETEMPLATE_H
#define IMAGETEMPLATE_H

#include <QWidget>                                // For widget base class
#include <QJsonObject>                            // For JSON object handling
#include <QTableWidget>                           // For table widget

// %%% Class Definition %%%
/* Widget for image template management */
class ImageTemplate : public QWidget
{
    Q_OBJECT

public:
    // Initialize image template
    explicit ImageTemplate(QWidget *parent = nullptr);
    // Setup image cell in table
    void setupImageCell(int row, const QString &fullKey, const QJsonObject &obj, QTableWidget *tableWidget);
    // Set connected ID
    void setConnectedID(const QString &id) { connectedID = id; }
    // Set template name
    void setName(const QString &n) { name = n; }
    // Constant for row height
    static constexpr int ROW_HEIGHT = 100;
    // Constant for image size
    static constexpr int IMAGE_SIZE = 60;

signals:
    // Signal value change
    void valueChanged(QString ID, QString name, QJsonObject delta);

private:
    // %%% Data Members %%%
    // Connected item ID
    QString connectedID;
    // Template name
    QString name;
};

#endif // IMAGETEMPLATE_H

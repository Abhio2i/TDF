

/* ========================================================================= */
/* File: colortemplate.h                                                    */
/* Purpose: Defines widget for managing color templates                      */
/* ========================================================================= */

#ifndef COLORTEMPLATE_H
#define COLORTEMPLATE_H

#include <QWidget>                                // For widget base class
#include <QJsonObject>                            // For JSON object handling
#include <QPushButton>                            // For push button widget
#include <QTableWidget>                           // For table widget

// Forward declaration
class Inspector;

// %%% Class Definition %%%
/* Widget for color template management */
class ColorTemplate : public QWidget
{
    Q_OBJECT

public:
    // Initialize color template with Inspector reference
    explicit ColorTemplate(Inspector *inspector, QWidget *parent = nullptr);  // CHANGED
    // Setup color cell in table
    void setupColorCell(int row, const QString &fullKey, const QJsonObject &obj, QTableWidget *tableWidget);
    // Set connected ID
    void setConnectedID(const QString &id) { connectedID = id; }
    // Set template name
    void setName(const QString &n) { name = n; }
    // Constant for row height
    static constexpr int ROW_HEIGHT = 30;
       void setMainID(const QString &id) { mainID = id; }

signals:
    // Signal value change
    void valueChanged(QString ID, QString name, QJsonObject delta);

private:
    // %%% Data Members %%%
    // Connected item ID
    QString connectedID;
    // Template name
    QString name;
    // Inspector reference
    Inspector *inspectorRef;
     QString mainID;
};

#endif // COLORTEMPLATE_H

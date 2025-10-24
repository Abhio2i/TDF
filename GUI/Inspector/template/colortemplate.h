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

// %%% Class Definition %%%
/* Widget for color template management */
class ColorTemplate : public QWidget
{
    Q_OBJECT

public:
    // Initialize color template
    explicit ColorTemplate(QWidget *parent = nullptr);
    // Setup color cell in table
    void setupColorCell(int row, const QString &fullKey, const QJsonObject &obj, QTableWidget *tableWidget);
    // Set connected ID
    void setConnectedID(const QString &id) { connectedID = id; }
    // Set template name
    void setName(const QString &n) { name = n; }
    // Constant for row height
    static constexpr int ROW_HEIGHT = 30;

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

#endif // COLORTEMPLATE_H

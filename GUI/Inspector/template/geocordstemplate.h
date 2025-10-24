/* ========================================================================= */
/* File: geocordstemplate.h                                                 */
/* Purpose: Defines widget for managing geocoordinates templates             */
/* ========================================================================= */

#ifndef GEOCORDSTEMPLATE_H
#define GEOCORDSTEMPLATE_H

#include <QWidget>                                // For widget base class
#include <QJsonObject>                            // For JSON object handling
#include <QTableWidget>                           // For table widget

// %%% Class Definition %%%
/* Widget for geocoordinates template management */
class GeocordsTemplate : public QWidget
{
    Q_OBJECT

public:
    // Initialize geocoordinates template
    explicit GeocordsTemplate(QWidget *parent = nullptr);
    // Setup geocoordinates cell in table
    void setupGeocordsCell(int row, const QString &fullKey, const QJsonObject &obj, QTableWidget *tableWidget);
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

#endif // GEOCORDSTEMPLATE_H

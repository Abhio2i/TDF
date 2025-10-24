/* ========================================================================= */
/* File: vectortemplate.h                                                   */
/* Purpose: Defines widget for managing vector templates                     */
/* ========================================================================= */

#ifndef VECTORTEMPLATE_H
#define VECTORTEMPLATE_H

#include <QWidget>                                // For widget base class
#include <QJsonObject>                            // For JSON object handling
#include <QTableWidget>                           // For table widget
#include "GUI/Inspector/inspector.h"              // For inspector panel

// %%% Class Definition %%%
/* Widget for vector template management */
class VectorTemplate : public QWidget
{
    Q_OBJECT

public:
    // Initialize vector template
    explicit VectorTemplate(QWidget *parent = nullptr);
    // Setup vector cell in table
    void setupVectorCell(int row, const QString &fullKey, const QJsonObject &obj, QTableWidget *tableWidget);
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
    // Copied vector data
    QJsonObject copiedVectorData;
};

#endif // VECTORTEMPLATE_H

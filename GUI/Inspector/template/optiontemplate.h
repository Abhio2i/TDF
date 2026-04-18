
/* ========================================================================= */
/* File: optiontemplate.h                                                   */
/* Purpose: Defines widget for managing option templates                     */
/* Written by   : Arti Rajpoot                                               */
/* ========================================================================= */

#ifndef OPTIONTEMPLATE_H
#define OPTIONTEMPLATE_H

#include <QWidget>                                // For widget base class
#include <QJsonObject>                            // For JSON object handling
#include <QJsonArray>                             // For JSON array handling
#include <QTableWidget>                           // For table widget
#include <QComboBox>                              // For combo box widget

// Forward declaration
class Inspector;

// %%% Class Definition %%%
/* Widget for option template management */
class OptionTemplate : public QWidget
{
    Q_OBJECT

public:
    // Initialize option template
    explicit OptionTemplate(QWidget *parent = nullptr);
    // Setup option cell in table
    void setupOptionCell(int row, const QString &fullKey, const QJsonObject &obj, QTableWidget *tableWidget);
    // Set connected ID
    void setConnectedID(const QString &id) { connectedID = id; }
    // Set template name
    void setName(const QString &n) { name = n; }
    // Set main ID
    void setMainID(const QString &id) { mainID = id; }
    // Set inspector reference
    void setInspectorRef(Inspector *inspector) { inspectorRef = inspector; }
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
    // Main ID
    QString mainID;
    // Inspector reference
    Inspector *inspectorRef;
};

#endif // OPTIONTEMPLATE_H

/* =============================================================================
 * FILE:         imagetemplate.h
 * MODULE:       Image Template Manager
 * PROJECT:      Indigenous Scenario and Sensor Simulation Toolkit (ISSST)
 * ORGANISATION: Oxygen 2 Innovation (O2I).
 * STANDARD:     RTCA DO-178C / ED-12C, DAL B
 * COVERAGE:     Branch / Decision Coverage required (100% true/false paths)
 *
 * DESCRIPTION:  Declares the ImageTemplate class which provides a widget for
 *               managing image templates. It interfaces with the Inspector
 *               panel to set up image cells in a table, maintain connected
 *               entity IDs, template names, and emit value changes when an
 *               image is modified. Supports larger row heights and image
 *               thumbnails.
 *
 * REQUIREMENTS: REQ-IMAGE-010  Image template management widget
 *               REQ-IMAGE-011  Setup image cell in table widget
 *               REQ-IMAGE-012  Maintain connected ID and template name
 *               REQ-IMAGE-013  Signal valueChanged on image modification
 *               REQ-IMAGE-014  Support configurable row height and image size
 *
 * AUTHOR:       Arti Rajpoot
 * REVIEWED BY:  [Reviewer Name], [Review Date] — SPR-IMAGE-001
 *
 *
 * COPYRIGHT:    Oxygen 2 Innovation (O2I). All rights reserved.
 *               Restricted circulation — defence simulation use only.
 * =============================================================================
 */

#ifndef IMAGETEMPLATE_H
#define IMAGETEMPLATE_H

#include <QWidget>                                // For widget base class
#include <QJsonObject>                            // For JSON object handling
#include <QTableWidget>                           // For table widget

// Forward declaration
class Inspector;

// %%% Class Definition %%%
/* Widget for image template management */
class ImageTemplate : public QWidget
{
    Q_OBJECT

public:
    // Initialize image template with Inspector reference
    explicit ImageTemplate(Inspector *inspector, QWidget *parent = nullptr);
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

#endif // IMAGETEMPLATE_H

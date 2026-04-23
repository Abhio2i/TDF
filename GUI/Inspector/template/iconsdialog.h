
/* =============================================================================
 * FILE:         iconsdialog.h
 * MODULE:       Icons Selection Dialog
 * PROJECT:      Indigenous Scenario and Sensor Simulation Toolkit (ISSST)
 * ORGANISATION: Oxygen 2 Innovation (O2I).
 * STANDARD:     RTCA DO-178C / ED-12C, DAL B
 * COVERAGE:     Branch / Decision Coverage required (100% true/false paths)
 *
 * DESCRIPTION:  Declares the IconsDialog class which provides a modal dialog
 *               for selecting images from application resources. The dialog
 *               displays a list of available icons, supports searching/filtering,
 *               and returns the selected image path. It integrates with the
 *               Inspector panel to assign icons to entities or components.
 *
 * REQUIREMENTS: REQ-ICON-010  Icon selection dialog
 *               REQ-ICON-011  Display all images from resource prefixes
 *               REQ-ICON-012  Search/filter images by name
 *               REQ-ICON-013  Return selected image path
 *               REQ-ICON-014  Integration with Inspector panel
 *
 * AUTHOR:       Arti Rajpoot
 * REVIEWED BY:  [Reviewer Name], [Review Date] — SPR-ICON-001
 *
 *
 * COPYRIGHT:    Oxygen 2 Innovation (O2I). All rights reserved.
 *               Restricted circulation — defence simulation use only.
 * =============================================================================
 */

#ifndef ICONSDIALOG_H
#define ICONSDIALOG_H

#include <QDialog>
#include <QListWidget>

// Forward declaration
class Inspector;

class IconsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit IconsDialog(QWidget *parent = nullptr);
    QString selectedImagePath() const;
    void setMainID(const QString &id) { mainID = id; }
    void setInspectorRef(Inspector *inspector) { inspectorRef = inspector; }
     void setSearchFilter(const QString &filter);
private slots:
    void onSearchTextChanged(const QString &text);
private:
    void loadAllImagesAutomatically();
    void scanResourcePrefix(const QString &prefix, const QStringList &extensions);
    bool addImageToList(const QString &imagePath, const QString &fileName);
    void filterImages(const QString &searchText);
    QListWidget *listWidget;
    QLineEdit *searchBox;
    QString mainID;
    Inspector *inspectorRef;
    QList<QPair<QString, QString>> allImages;
    QString m_selectedPath;

};

#endif // ICONSDIALOG_H


/* ========================================================================= */
/* File: iconsdialog.h                                                      */
/* Purpose: Dialog for selecting images from resources                       */
/* ========================================================================= */

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

private:
    void loadAllImagesAutomatically();
    void scanResourcePrefix(const QString &prefix, const QStringList &extensions);
    bool addImageToList(const QString &imagePath, const QString &fileName);

    QListWidget *listWidget;
    QString mainID;
    Inspector *inspectorRef;
};

#endif // ICONSDIALOG_H

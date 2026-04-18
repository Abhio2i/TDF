
/* ========================================================================= */
/* File: iconsdialog.h                                                      */
/* Purpose: Dialog for selecting images from resources                       */
/* Written by   : Arti Rajpoot                                               */
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

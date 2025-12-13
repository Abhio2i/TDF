// recentprojectsdialog.h
#ifndef RECENTPROJECTSDIALOG_H
#define RECENTPROJECTSDIALOG_H

#include <QDialog>
#include <QListWidget>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>

class RecentProjectsDialog : public QDialog
{
    Q_OBJECT
public:
    explicit RecentProjectsDialog(QWidget *parent = nullptr);
    QString getSelectedFile() const { return selectedFile; }

    void setRecentProjects(const QStringList &projects);

private slots:
    void onItemDoubleClicked(QListWidgetItem *item);
    void onOpenClicked();
    void onCancelClicked();

private:
    QListWidget *listWidget;
    QPushButton *openButton;
    QPushButton *cancelButton;
    QString selectedFile;
};

#endif // RECENTPROJECTSDIALOG_H

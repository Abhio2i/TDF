
#ifndef SIDEBARWIDGET_H
#define SIDEBARWIDGET_H

#include <QWidget>
#include <QHBoxLayout>
#include <QPushButton>
#include <QButtonGroup>

class SidebarWidget : public QWidget {
    Q_OBJECT

public:
    explicit SidebarWidget(QWidget *parent = nullptr);
    void setActiveButton(const QString &viewName);

signals:
    void viewSelected(const QString &viewName);

private:
    QPushButton* createSidebarButton(const QString &text, const QString &viewName);
    QButtonGroup *buttonGroup;
};

#endif // SIDEBARWIDGET_H

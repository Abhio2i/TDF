/* Header guard section */
#ifndef SIDEBARWIDGET_H
#define SIDEBARWIDGET_H

/* Includes section */
#include <QWidget>
#include <QHBoxLayout>
#include <QPushButton>
#include <QButtonGroup>

/* Class declaration section */
class SidebarWidget : public QWidget {
    /* Qt meta-object section */
    Q_OBJECT

public:
    /* Constructor section */
    explicit SidebarWidget(QWidget *parent = nullptr);

    /* Button management section */
    void setActiveButton(const QString &viewName);

signals:
    /* Signals section */
    void viewSelected(const QString &viewName);

private:
    /* Private methods section */
    QPushButton* createSidebarButton(const QString &text, const QString &viewName);

    /* Private members section */
    QButtonGroup *buttonGroup;
};

/* End of header guard section */
#endif // SIDEBARWIDGET_H


#ifndef NAVIGATIONPAGE_H
#define NAVIGATIONPAGE_H

#include <QWidget>
#include <QPushButton>
#include <QToolButton>

class NavigationPage : public QWidget
{
    Q_OBJECT

public:
    explicit NavigationPage(QWidget *parent = nullptr);

signals:
    void editorRequested(const QString &editorKey);

private:
    QList<QToolButton*> navButtons;
    QToolButton* activeButton = nullptr;

    QToolButton* createNavButton(const QString &iconPath, const QString &label, const QString &editorKey);

    void setActiveButton(QToolButton* button);
};

#endif // NAVIGATIONPAGE_H

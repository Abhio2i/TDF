// docktitlemenu.h
#ifndef DOCKTITLEMENU_H
#define DOCKTITLEMENU_H

#include <QObject>
#include <QPushButton>
#include <QDockWidget>
#include <QMenu>

class DockTitleMenu : public QObject
{
    Q_OBJECT
public:
    explicit DockTitleMenu(QDockWidget *parentDock, QObject *parent = nullptr);
    void setupMenu(QPushButton *menuButton);

signals:
    void lockRequested(bool locked);
    void copyRequested();
    void pasteRequested();
    void addInspectorRequested();

private:
    QDockWidget *m_dock;
    QMenu *m_menu;
};

class HoverEventFilter : public QObject
{
    Q_OBJECT
public:
    HoverEventFilter(QPushButton *button, QObject *parent = nullptr);

protected:
    bool eventFilter(QObject *obj, QEvent *event) override;

private:
    QPushButton *m_button;
};

#endif // DOCKTITLEMENU_H

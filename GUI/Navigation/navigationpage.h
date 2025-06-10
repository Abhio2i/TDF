
#ifndef NAVIGATIONPAGE_H
#define NAVIGATIONPAGE_H

#include <QWidget>
#include <QPushButton>

class NavigationPage : public QWidget {
    Q_OBJECT

public:
    explicit NavigationPage(QWidget *parent = nullptr);

private:
    QPushButton* createNavButton(const QString &iconPath, const QString &label, const QString &editorKey);
    void openEditorWindow(const QString &editorKey);
};

#endif // NAVIGATIONPAGE_H

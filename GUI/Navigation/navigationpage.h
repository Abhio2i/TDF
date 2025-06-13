/* Header guard section */
#ifndef NAVIGATIONPAGE_H
#define NAVIGATIONPAGE_H

/* Includes section */
#include <QWidget>
#include <QPushButton>

/* Class declaration section */
class NavigationPage : public QWidget {
    /* Qt meta-object section */
    Q_OBJECT

public:
    /* Constructor section */
    explicit NavigationPage(QWidget *parent = nullptr);

private:
    /* Private methods section */
    QPushButton* createNavButton(const QString &iconPath, const QString &label, const QString &editorKey);
    void openEditorWindow(const QString &editorKey);
};

/* End of header guard section */
#endif // NAVIGATIONPAGE_H

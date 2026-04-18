#ifndef PROFILEINFODIALOG_TEST_H
#define PROFILEINFODIALOG_TEST_H

#include <QObject>

class ProfileInfoDialog;

class TestProfileInfoDialog : public QObject
{
    Q_OBJECT

private slots:
    void init();
    void cleanup();

    void testDialogProperties();
    void testTitleLabel();
    void testTextEdit();
    void testButtonsExist();

    void testCloseButtonExists();
    void testRefreshTimer();
    void testStaticShowMethod();
    void testDialogFlags();
    void testStatusLabel();

private:
    ProfileInfoDialog* dialog = nullptr;
};

#endif

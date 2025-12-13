

#ifndef PROFILEINFODIALOG_H
#define PROFILEINFODIALOG_H

#include "qelapsedtimer.h"
#include <QDialog>
#include <QTimer>
#include <QTextEdit>
#include <QLabel>
#include <QPushButton>

class ProfileInfoDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ProfileInfoDialog(QWidget *parent = nullptr);
    ~ProfileInfoDialog();

    // Static function to show dialog
    static void showProfileInfo(QWidget *parent = nullptr);

protected:
    void closeEvent(QCloseEvent *event) override;

private slots:
    void updateRealTimeInfo();
    void copyToClipboard();

private:
    void setupUI();
    QString getPerformanceMetrics();

    // UI elements
    QTextEdit* textEdit;
    QLabel* titleLabel;
    QPushButton* copyButton;
    QPushButton* closeButton;

    // Timer
    QTimer* refreshTimer;
    int frameCount;
    int fps;
    QElapsedTimer fpsTimer;
};

#endif // PROFILEINFODIALOG_H

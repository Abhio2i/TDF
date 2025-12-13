#ifndef LOADINGDIALOG_H
#define LOADINGDIALOG_H

#include <QDialog>
#include <QLabel>
#include <QProgressBar>
#include <QVBoxLayout>
#include <QTimer>

class LoadingDialog : public QDialog
{
    Q_OBJECT

public:
    explicit LoadingDialog(QWidget *parent = nullptr, const QString &title = "Loading...");
    ~LoadingDialog();

    void setMessage(const QString &message);
    void setProgress(int value, int maximum = 100);
    void showWithDelay(int delayMs = 100);
    void closeWithDelay(int delayMs = 100);

public slots:
    void updateProgress(int value);
    void setMaximum(int maximum);
    void showMessage(const QString &message);

private:
    QLabel *messageLabel;
    QProgressBar *progressBar;
    QTimer *showTimer;
};

#endif // LOADINGDIALOG_H

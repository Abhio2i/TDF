#include "LoadingDialog.h"
#include <QApplication>
#include <QDesktopWidget>

LoadingDialog::LoadingDialog(QWidget *parent, const QString &title)
    : QDialog(parent)
{
    setWindowTitle(title);
    setWindowFlags(Qt::Dialog | Qt::FramelessWindowHint);
    setModal(true);
    setFixedSize(300, 100);

    QVBoxLayout *layout = new QVBoxLayout(this);

    messageLabel = new QLabel("Loading data...", this);
    messageLabel->setAlignment(Qt::AlignCenter);
    layout->addWidget(messageLabel);

    progressBar = new QProgressBar(this);
    progressBar->setRange(0, 100);
    progressBar->setTextVisible(true);
    progressBar->setFormat("%p%");
    layout->addWidget(progressBar);

    setLayout(layout);

    // Center on screen
    QRect screenGeometry = QApplication::primaryScreen()->geometry();
    int x = (screenGeometry.width() - width()) / 2;
    int y = (screenGeometry.height() - height()) / 2;
    move(x, y);

    showTimer = new QTimer(this);
    showTimer->setSingleShot(true);
}

LoadingDialog::~LoadingDialog()
{
    if (showTimer && showTimer->isActive()) {
        showTimer->stop();
    }
}

void LoadingDialog::setMessage(const QString &message)
{
    messageLabel->setText(message);
}

void LoadingDialog::setProgress(int value, int maximum)
{
    if (maximum > 0) {
        progressBar->setMaximum(maximum);
    }
    progressBar->setValue(value);
}

void LoadingDialog::showWithDelay(int delayMs)
{
    showTimer->start(delayMs);
    connect(showTimer, &QTimer::timeout, this, &LoadingDialog::show);
}

void LoadingDialog::closeWithDelay(int delayMs)
{
    QTimer::singleShot(delayMs, this, &LoadingDialog::close);
}

void LoadingDialog::updateProgress(int value)
{
    progressBar->setValue(value);
    QApplication::processEvents(); // UI update ke liye
}

void LoadingDialog::setMaximum(int maximum)
{
    progressBar->setMaximum(maximum);
}

void LoadingDialog::showMessage(const QString &message)
{
    messageLabel->setText(message);
    QApplication::processEvents();
}

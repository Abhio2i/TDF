
#include "profileinfodialog.h"
#include "core/Debug/profiler.h"
#include <QApplication>
#include <QClipboard>
#include <QDateTime>
#include <QTimer>
#include <QElapsedTimer>
#include <QTextStream>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QTextEdit>
#include <QPushButton>
#include <QFont>
#include <QCloseEvent>
#include <QScreen>
#include <QDebug>

ProfileInfoDialog::ProfileInfoDialog(QWidget *parent)
    : QDialog(parent)
    , frameCount(0)
    , fps(0)
{
    setWindowTitle("Premetrix Performance Metrics");
    setMinimumSize(400, 500);

    // Non-modal dialog
    setWindowFlags(Qt::Tool | Qt::Dialog | Qt::WindowCloseButtonHint);

    setupUI();


    fpsTimer.start();


    refreshTimer = new QTimer(this);
    connect(refreshTimer, &QTimer::timeout, this, &ProfileInfoDialog::updateRealTimeInfo);
    refreshTimer->start(100); // 100ms = 10 FPS


    QTimer* fpsUpdateTimer = new QTimer(this);
    connect(fpsUpdateTimer, &QTimer::timeout, this, [this]() {
        fps = frameCount * 10;
        frameCount = 0;
    });
    fpsUpdateTimer->start(1000);

    // Initial update
    updateRealTimeInfo();

    qDebug() << "ProfileInfoDialog created with CanvasWidget-style updates";
}

ProfileInfoDialog::~ProfileInfoDialog()
{
    if (refreshTimer) {
        refreshTimer->stop();
        refreshTimer->deleteLater();
    }
}

void ProfileInfoDialog::setupUI()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    // Title
    titleLabel = new QLabel("Premetrix Performance Metrics", this);
    titleLabel->setAlignment(Qt::AlignCenter);
    titleLabel->setStyleSheet("color: #2c3e50; font-size: 16px; font-weight: bold; margin: 10px;");
    mainLayout->addWidget(titleLabel);


    textEdit = new QTextEdit(this);
    textEdit->setReadOnly(true);
    textEdit->setFont(QFont("Consolas", 9));
    textEdit->setStyleSheet(
        "QTextEdit {"
        "  background-color: #f8f9fa;"
        "  color: #212529;"
        "  border: 1px solid #dee2e6;"
        "  border-radius: 4px;"
        "  padding: 8px;"
        "}"
        );
    mainLayout->addWidget(textEdit);

    // Button layout
    QHBoxLayout *buttonLayout = new QHBoxLayout();

    copyButton = new QPushButton("Copy", this);
    copyButton->setStyleSheet(
        "QPushButton {"
        "  background-color: #6c757d;"
        "  color: white;"
        "  padding: 6px 12px;"
        "  border-radius: 4px;"
        "  font-weight: bold;"
        "}"
        "QPushButton:hover {"
        "  background-color: #5a6268;"
        "}"
        );
    copyButton->setToolTip("Copy metrics to clipboard");
    connect(copyButton, &QPushButton::clicked, this, &ProfileInfoDialog::copyToClipboard);

    closeButton = new QPushButton("Close", this);
    closeButton->setStyleSheet(
        "QPushButton {"
        "  background-color: #dc3545;"
        "  color: white;"
        "  padding: 6px 12px;"
        "  border-radius: 4px;"
        "  font-weight: bold;"
        "}"
        "QPushButton:hover {"
        "  background-color: #c82333;"
        "}"
        );
    connect(closeButton, &QPushButton::clicked, this, &ProfileInfoDialog::close);

    buttonLayout->addWidget(copyButton);
    buttonLayout->addStretch();
    buttonLayout->addWidget(closeButton);

    mainLayout->addLayout(buttonLayout);

    // Status label
    QLabel *statusLabel = new QLabel("🔄 Live updating every 100ms", this);
    statusLabel->setAlignment(Qt::AlignCenter);
    statusLabel->setStyleSheet("color: #6c757d; font-size: 10px; font-style: italic; padding: 5px;");
    mainLayout->addWidget(statusLabel);

    setLayout(mainLayout);
}

void ProfileInfoDialog::updateRealTimeInfo()
{
    frameCount++;

    QString info;
    QTextStream stream(&info);


    stream << "=== PREMETRIX PERFORMANCE METRICS ===\n";
    stream << "Timestamp: " << QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss.zzz") << "\n";
    stream << "=====================================\n\n";


    if (Profiler::currentFrame) {
        const auto currentFrame = Profiler::currentFrame;


        struct PerfMetric {
            const char* label;
            int timeValue;
        };

        const PerfMetric metrics[] = {
            {"Exc Time", currentFrame->excutionTime},
            {"Can Time", currentFrame->canvasTime},
            {"Phy Time", currentFrame->physicsTime},
            {"Dym Time", currentFrame->dynamicTime},
            {"Sen Time", currentFrame->SensorTime},
            {"Rdr Time", currentFrame->RadarTime},
            {"EW Time",  currentFrame->EWTime},
            {"CSM Time", currentFrame->CSMTime},
            {"ESM Time", currentFrame->ESMTime},
            {"IFF Time", currentFrame->IFFTime},
            {"Rdo Time", currentFrame->RadioTime},
            {"CSM UI", currentFrame->CSMTime},
            {"ESM UI", currentFrame->ESMTime},
            {"GUI Time", currentFrame->GUITime}
        };

        stream << "Performance Metrics:\n";
        stream << "-------------------\n";

        for (const auto& metric : metrics) {
            QString text = QString("%1 %2ms")
                               .arg(metric.label)
                               .arg(metric.timeValue);
            stream << text << "\n";
        }
    } else {
        stream << "Performance Metrics:\n";
        stream << "-------------------\n";
        stream << "Exc Time:     0 ms\n";
        stream << "Can Time:     0 ms\n";
        stream << "Phy Time:     0 ms\n";
        stream << "Dym Time:     0 ms\n";
        stream << "Sen Time:     0 ms\n";
        stream << "Rdr Time:     0 ms\n";
        stream << "EW Time:      0 ms\n";
        stream << "CSM Time:     0 ms\n";
        stream << "ESM Time:     0 ms\n";
        stream << "IFF Time:     0 ms\n";
        stream << "Rdo Time:     0 ms\n";
        stream << "CSM UI:       0 ms\n";
        stream << "ESM UI:       0 ms\n";
        stream << "GUI Time:     0 ms\n";
    }

    textEdit->setPlainText(info);

    QString timestamp = QDateTime::currentDateTime().toString("hh:mm:ss");
    setWindowTitle(QString("Performance Metrics - %1").arg(timestamp));
}

void ProfileInfoDialog::copyToClipboard()
{
    QClipboard *clipboard = QApplication::clipboard();
    clipboard->setText(textEdit->toPlainText());

    // Visual feedback
    copyButton->setText("✓ Copied!");
    QTimer::singleShot(1000, this, [=]() {
        copyButton->setText("Copy");
    });
}

void ProfileInfoDialog::closeEvent(QCloseEvent *event)
{
    if (refreshTimer) {
        refreshTimer->stop();
    }

    QDialog::closeEvent(event);
}

// Static function
void ProfileInfoDialog::showProfileInfo(QWidget *parent)
{
    static ProfileInfoDialog *dialog = nullptr;

    if (!dialog || !dialog->isVisible()) {
        dialog = new ProfileInfoDialog(parent);
        dialog->setAttribute(Qt::WA_DeleteOnClose);

        if (parent) {
            QRect parentRect = parent->geometry();
            QRect dialogRect = dialog->geometry();
            dialogRect.moveCenter(parentRect.center());
            dialog->move(dialogRect.topLeft());
        }
        connect(dialog, &QDialog::finished, []() {
            dialog = nullptr;
        });
    }

    dialog->show();
    // dialog->raise();
    // dialog->activateWindow();

    qDebug() << "ProfileInfoDialog shown with CanvasWidget-style metrics";
}

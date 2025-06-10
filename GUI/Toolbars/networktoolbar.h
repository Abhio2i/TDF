
#ifndef NETWORKTOOLBAR_H
#define NETWORKTOOLBAR_H

#include <QToolBar>
#include <QAction>
#include <QDialog>
#include <QMessageBox>
#include <QLineEdit>
#include <QComboBox>
#include <QTableWidget>
#include <QTextEdit>
#include <QTimer>
#include <QDockWidget>
#include <QPainter>
#include <QPushButton>
#include <QAbstractButton>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

class NetworkManager;

class NetworkToolbar : public QToolBar
{
    Q_OBJECT

public:
    explicit NetworkToolbar(QWidget *parent = nullptr);
    ~NetworkToolbar();

private slots:
    void startSession();
    void stopSession();
    void connectClient();
    void disconnectClient();
    void viewNetworkStatus();
    void openPacketAnalyzer();
    void runProtocolTest();
    void configureSyncSettings();
    void viewLogs();
    void simulatePDU();
    void updatePacketTable();

private:
    NetworkManager *networkManager;
    QAction *startAction;
    QAction *stopAction;
    QAction *connectAction;
    QAction *disconnectAction;
    QAction *statusAction;
    QAction *packetAction;
    QAction *testAction;
    QAction *syncAction;
    QAction *logAction;
    QAction *pduAction;
    QTableWidget *packetTable;
    QTimer *packetTimer;
    QJsonObject config;

    bool loadConfig(const QString &filePath);
    void setupDialog(QDialog *dialog, const QString &title);
    void showMessage(const QString &title, const QString &message, bool isError = false);
    QPixmap withWhiteBg(const QString &iconPath);
    void createActions();
};

#endif // NETWORKTOOLBAR_H

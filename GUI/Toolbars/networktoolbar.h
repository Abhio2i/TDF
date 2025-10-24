/* ========================================================================= */
/* File: networktoolbar.h                                                   */
/* Purpose: Defines toolbar for network management and monitoring            */
/* ========================================================================= */

#ifndef NETWORKTOOLBAR_H
#define NETWORKTOOLBAR_H

#include <QToolBar>                               // For toolbar base class
#include <QAction>                                // For action items
#include <QDialog>                                // For dialog widget
#include <QMessageBox>                            // For message box
#include <QLineEdit>                              // For text input widget
#include <QComboBox>                              // For combo box widget
#include <QTableWidget>                           // For table widget
#include <QTextEdit>                              // For text edit widget
#include <QTimer>                                 // For timer functionality
#include <QDockWidget>                            // For dock widget
#include <QPainter>                               // For painting operations
#include <QPushButton>                            // For push button widget
#include <QAbstractButton>                        // For abstract button
#include <QJsonDocument>                          // For JSON document handling
#include <QJsonObject>                            // For JSON object handling
#include <QJsonArray>                             // For JSON array handling
#include <QVBoxLayout>                            // For vertical layout
#include <core/Network/networkmanager.h>          // For network manager

// %%% Class Definition %%%
/* Toolbar for network operations */
class NetworkToolbar : public QToolBar
{
    Q_OBJECT

public:
    // Initialize toolbar
    explicit NetworkToolbar(QWidget *parent = nullptr);
    // Clean up resources
    ~NetworkToolbar();
    // Set network manager
    void setNetworkManager(NetworkManager *netmger);
    // Enable message sending UI
    void enableMessageSendingUI();

private slots:
    // Start network session
    void startSession();
    // Stop network session
    void stopSession();
    // Connect client
    void connectClient();
    // Disconnect client
    void disconnectClient();
    // View network status
    void viewNetworkStatus();
    // Open packet analyzer
    void openPacketAnalyzer();
    // Run protocol test
    void runProtocolTest();
    // Configure sync settings
    void configureSyncSettings();
    // View logs
    void viewLogs();
    // Simulate PDU
    void simulatePDU();
    // Update packet table
    void updatePacketTable();
    // Update network status
    void updateNetwork();

private:
    // %%% UI Components %%%
    // Network manager instance
    NetworkManager *networkManager;
    // Start action
    QAction *startAction;
    // Stop action
    QAction *stopAction;
    // Connect action
    QAction *connectAction;
    // Disconnect action
    QAction *disconnectAction;
    // Status action
    QAction *statusAction;
    // Packet analyzer action
    QAction *packetAction;
    // Protocol test action
    QAction *testAction;
    // Sync settings action
    QAction *syncAction;
    // Log action
    QAction *logAction;
    // PDU simulation action
    QAction *pduAction;
    // Refresh action
    QAction *refreshAction;
    // Packet table widget
    QTableWidget *packetTable;
    // Packet update timer
    QTimer *packetTimer;
    // Configuration object
    QJsonObject config;
    // Main window layout
    QVBoxLayout *mainWindowLayout;

    // %%% Utility Methods %%%
    // Load configuration
    bool loadConfig(const QString &filePath);
    // Setup dialog
    void setupDialog(QDialog *dialog, const QString &title);
    // Show message
    void showMessage(const QString &title, const QString &message, bool isError = false);
    // Create pixmap with white background
    QPixmap withWhiteBg(const QString &iconPath);
    // Create toolbar actions
    void createActions();
};

#endif // NETWORKTOOLBAR_H

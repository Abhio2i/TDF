/* ========================================================================= */
/* File: networktoolbar.h                                                   */
/* Purpose: Defines toolbar for network management and monitoring            */
//               Written by Aarti Rajpoot
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
    void enableMessageSendingUI();
private slots:
    // Start network session
    void startSession();
    // Stop network session
    void stopSession();
    void viewNetworkStatus();
    // Open packet analyzer
    void openPacketAnalyzer();
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
    QAction *statusAction;
    QTableWidget *packetTable;
    // Packet update timer
    QTimer *packetTimer;
    // Configuration object
    QJsonObject config;
    QVBoxLayout *mainWindowLayout;
    // %%% Utility Methods %%%
    // Load configuration
    bool loadConfig(const QString &filePath);
    void setupDialog(QDialog *dialog, const QString &title);
    void showMessage(const QString &title, const QString &message, bool isError = false);
    QPixmap withWhiteBg(const QString &iconPath);
    // Create toolbar actions
    void createActions();
};

#endif // NETWORKTOOLBAR_H

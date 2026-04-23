/* =============================================================================
 * FILE:         networktoolbar.h
 * MODULE:       Network Toolbar
 * PROJECT:      Indigenous Scenario and Sensor Simulation Toolkit (ISSST)
 * ORGANISATION: Oxygen 2 Innovation (O2I).
 * STANDARD:     RTCA DO-178C / ED-12C, DAL B
 * COVERAGE:     Branch / Decision Coverage required (100% true/false paths)
 *
 * DESCRIPTION:  Declares the NetworkToolbar class which provides a toolbar for
 *               network management and monitoring. It includes actions to start
 *               and stop network sessions, view network status, and open a packet
 *               analyzer. Integrates with NetworkManager to handle network
 *               operations and displays packet data in a table with periodic
 *               updates. Supports loading configuration from JSON.
 *
 * REQUIREMENTS: REQ-NETTOOLBAR-010  Network toolbar with start/stop session
 *               REQ-NETTOOLBAR-011  View network status action
 *               REQ-NETTOOLBAR-012  Packet analyzer with table and timer updates
 *               REQ-NETTOOLBAR-013  Integration with NetworkManager
 *               REQ-NETTOOLBAR-014  Load configuration from JSON file
 *               REQ-NETTOOLBAR-015  Enable message sending UI
 *
 * AUTHOR:       Aarti Rajpoot
 * REVIEWED BY:  [Reviewer Name], [Review Date] — SPR-NETTOOLBAR-001
 *
 *
 * COPYRIGHT:    Oxygen 2 Innovation (O2I). All rights reserved.
 *               Restricted circulation — defence simulation use only.
 * =============================================================================
 */

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

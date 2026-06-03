/* =============================================================================
 * FILE:         NetworkToolbar.h
 * MODULE:       DIS Network Toolbar
 * PROJECT:      Tactical Display Framework (TDF)
 * ORGANISATION: Oxygen 2 Innovation (O2I)
 *
 * DESCRIPTION:  DIS network toolbar with settings dialog.
 *               Provides Connect/Disconnect/Settings actions.
 *               Opens DIS Settings dialog with tabs matching STAGE 22.0:
 *                 Connection tab — multicast, port, interface
 *                 Options tab    — exercise ID, site ID, app ID, DIS version
 *                 Messages tab   — which PDUs to send/receive
 *                 Status tab     — TX/RX rates, peer list, PDU log
 *
 * AUTHOR:       O2I Development Team
 * COPYRIGHT:    Oxygen 2 Innovation (O2I). All rights reserved.
 * =============================================================================
 */

#ifndef NETWORKTOOLBAR_H
#define NETWORKTOOLBAR_H

#include <QToolBar>
#include <QAction>
#include <QDialog>
#include <QTabWidget>
#include <QLabel>
#include <QLineEdit>
#include <QSpinBox>
#include <QCheckBox>
#include <QComboBox>
#include <QTableWidget>
#include <QTimer>
#include <QPushButton>
#include <QGroupBox>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGridLayout>
#include <QMessageBox>
#include <QPainter>

#include "core/DISPlugin/DISNetworkPlugin.h"

// =============================================================================
// NetworkToolbar
// Toolbar with Connect/Disconnect/Settings buttons
// Opens DIS settings dialog matching STAGE 22.0 UI
// =============================================================================
class NetworkToolbar : public QToolBar
{
    Q_OBJECT

public:
    explicit NetworkToolbar(QWidget* parent = nullptr);
    ~NetworkToolbar();

    void setDISPlugin(DISNetworkPlugin* plugin);

private slots:
    void onConnectClicked();
    void onDisconnectClicked();
    void onSettingsClicked();
    void onStatusClicked();
    void updateStatusIndicator();

private:
    // ── Plugin reference ──────────────────────────────────────────────────────
    DISNetworkPlugin* m_disPlugin = nullptr;

    // ── Toolbar actions ───────────────────────────────────────────────────────
    QAction* m_connectAction    = nullptr;
    QAction* m_disconnectAction = nullptr;
    QAction* m_settingsAction   = nullptr;
    QAction* m_statusAction     = nullptr;

    // ── Status indicator ──────────────────────────────────────────────────────
    QLabel*  m_statusLabel      = nullptr;
    QTimer*  m_statusTimer      = nullptr;

    // ── Helpers ───────────────────────────────────────────────────────────────
    void createActions();
    QPixmap withWhiteBg(const QString& iconPath);

    // ── Settings dialog tabs ──────────────────────────────────────────────────
    QWidget* createConnectionTab (DISConfig& config);
    QWidget* createOptionsTab    (DISConfig& config);
    //QWidget* createMessagesTab   ();
    QWidget* createMessagesTab   (DISConfig& config);  // ADD config parameter
    QWidget* createStatusTab     ();
};

#endif // NETWORKTOOLBAR_H

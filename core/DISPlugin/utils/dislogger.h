// =============================================================================
// FILE:        DISLogger.h
// MODULE:      DIS Network Plugin — Utils
// PROJECT:     Tactical Display Framework (TDF)
// ORGANISATION:Oxygen 2 Innovation (O2I)
//
// DESCRIPTION: Runtime log level control for the DIS plugin.
//              Replaces raw qDebug/qWarning calls with level-gated macros.
//              Log level can be changed at runtime without rebuilding.
//
// LEVELS:
//   None    — complete silence
//   Error   — critical failures only
//   Warning — errors + warnings
//   Basic   — lifecycle events (connect, entity add/remove, timeout)
//   Full    — everything including per-PDU traffic
//
// USAGE:
//   DIS_LOG_ERROR(   "[DISTransport] Bind failed: " << err);
//   DIS_LOG_WARNING( "[PDUSender] Entity not in mapper: " << id);
//   DIS_LOG_BASIC(   "[DISNetworkPlugin] Entity added: " << entityID);
//   DIS_LOG_FULL(    "[PDUSender] Sending EntityState for: " << id);
// =============================================================================

#ifndef DISLOGGER_H
#define DISLOGGER_H

#include <QDebug>
#include <QString>
#include <atomic>

// =============================================================================
// DISLogLevel
// =============================================================================
enum class DISLogLevel {
    None    = 0,
    Error   = 1,
    Warning = 2,
    Basic   = 3,
    Full    = 4
};

// =============================================================================
// DISLogger — singleton
// =============================================================================
class DISLogger
{
public:
    static DISLogger& instance()
    {
        static DISLogger s_instance;
        return s_instance;
    }

    void setLevel(DISLogLevel level)
    {
        m_level.store(static_cast<int>(level));
    }

    void setLevelFromString(const std::string& levelStr)
    {
        if      (levelStr == "None")    setLevel(DISLogLevel::None);
        else if (levelStr == "Error")   setLevel(DISLogLevel::Error);
        else if (levelStr == "Warning") setLevel(DISLogLevel::Warning);
        else if (levelStr == "Basic")   setLevel(DISLogLevel::Basic);
        else if (levelStr == "Full")    setLevel(DISLogLevel::Full);
        else                            setLevel(DISLogLevel::Basic);
    }

    DISLogLevel level() const
    {
        return static_cast<DISLogLevel>(m_level.load());
    }

    bool isEnabled(DISLogLevel level) const
    {
        return static_cast<int>(level) <= m_level.load();
    }

private:
    DISLogger() : m_level(static_cast<int>(DISLogLevel::Basic)) {}
    DISLogger(const DISLogger&)            = delete;
    DISLogger& operator=(const DISLogger&) = delete;

    std::atomic<int> m_level;
};

// =============================================================================
// Macros — use these everywhere instead of raw qDebug/qWarning
// =============================================================================

#define DIS_LOG_ERROR(msg) \
if (DISLogger::instance().isEnabled(DISLogLevel::Error)) \
    qCritical() << msg

#define DIS_LOG_WARNING(msg) \
    if (DISLogger::instance().isEnabled(DISLogLevel::Warning)) \
    qWarning() << msg

#define DIS_LOG_BASIC(msg) \
    if (DISLogger::instance().isEnabled(DISLogLevel::Basic)) \
    qDebug() << msg

#define DIS_LOG_FULL(msg) \
    if (DISLogger::instance().isEnabled(DISLogLevel::Full)) \
    qDebug() << msg

#endif // DISLOGGER_H

#include "airbase_api.h"
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonParseError>
#include <QDebug>
#include <QDir>

#if __has_include("Setup.h")
#  include "Setup.h"
#  define HAS_TDF_MANAGER 1
#else
#  define HAS_TDF_MANAGER 0
#endif

AirbaseApi* AirbaseApi::m_instance = nullptr;

AirbaseApi* AirbaseApi::instance()
{
    if (!m_instance)
        m_instance = new AirbaseApi();
    return m_instance;
}

AirbaseApi::AirbaseApi(QObject* parent)
    : QObject(parent)
{

}

AirbaseApi::~AirbaseApi() {}

void AirbaseApi::initialize()
{
    loadIcon();
}

void AirbaseApi::setIconPath(const QString& path)
{
    m_iconPath = path;
    loadIcon();
}

void AirbaseApi::loadIcon()
{
    m_icon = QPixmap(m_iconPath);
    if (m_icon.isNull())
        qWarning() << "AirbaseApi: failed to load icon" << m_iconPath;
}

static QString resolveAirbaseJsonPath()
{
#if HAS_TDF_MANAGER
    if (TDFManager::instance())
        return TDFManager::instance()->getAirbaseJsonPath();
#endif
    const QString home = QDir::homePath() + "/TDF/AirbaseData/airbase.json";
    if (QFile::exists(home))
        return home;
    return QStringLiteral(":/jsonData/DB/jsonData/airbase.json");
}

void AirbaseApi::loadAirbaseData()
{
    if (m_loaded && !m_positions.isEmpty())
        return;

    const QString path = resolveAirbaseJsonPath();
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning() << "AirbaseApi: cannot open" << path;
        return;
    }

    QJsonParseError err;
    QJsonDocument doc = QJsonDocument::fromJson(file.readAll(), &err);
    file.close();
    if (err.error != QJsonParseError::NoError || !doc.isObject()) {
        qWarning() << "AirbaseApi: JSON parse error" << err.errorString();
        return;
    }

    QJsonObject root = doc.object();
    if (!root.contains("airbases") || !root["airbases"].isArray()) {
        qWarning() << "AirbaseApi: missing airbases array";
        return;
    }

    m_positions.clear();
    const QJsonArray arr = root["airbases"].toArray();
    m_positions.reserve(arr.size());
    for (const QJsonValue& v : arr) {
        QJsonObject o = v.toObject();
        double lon = o["lon"].toDouble();
        double lat = o["lat"].toDouble();
        m_positions.append(QPointF(lon, lat));
    }
    m_loaded = true;
    emit dataLoaded(m_positions.size());
}

void AirbaseApi::setVisible(bool visible)
{
    if (visible == m_visible)
        return;
    if (visible)
        loadAirbaseData();
    m_visible = visible;
    emit visibilityChanged(m_visible);
}

void AirbaseApi::toggleVisibility()
{
    setVisible(!m_visible);
}

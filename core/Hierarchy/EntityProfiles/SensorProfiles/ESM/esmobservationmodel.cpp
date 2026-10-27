//============================================================================
// FILE:    esmobservationmodel.cpp
// MODULE:  ESM (Electronic Support Measures) Display
// PROJECT: Indigenous Scenario and Sensor Simulation Toolkit (ISSST)
//============================================================================
#include "esmobservationmodel.h"
#include "core/Hierarchy/EntityProfiles/platform.h"
#include <QBrush>
#include <QColor>
#include <QStringList>
#include <cmath>

namespace {

const QString kNA = QStringLiteral("--");
const QString kDeg = QStringLiteral("\u00B0");

QString bearingString(double deg)
{
    return QString("%1").arg(deg, 5, 'f', 1, QLatin1Char('0')) + kDeg;
}

QString rangeString(double km)
{
    return QString::number(km, 'f', 1) + QStringLiteral(" km");
}

} // namespace

ESMObservationModel::ESMObservationModel(QObject* parent)
    : QAbstractTableModel(parent)
{
}

int ESMObservationModel::rowCount(const QModelIndex& parent) const
{
    return parent.isValid() ? 0 : rows_.size();
}

int ESMObservationModel::columnCount(const QModelIndex& parent) const
{
    return parent.isValid() ? 0 : ColumnCount;
}

QString ESMObservationModel::cellText(const ESMObservation& o, int row, int column) const
{
    switch (column) {
    case ColIndex:      return QString::number(row + 1);
    case ColPlatformId: return o.platformId;
    case ColName:       return o.name.isEmpty() ? kNA : o.name;
    case ColEmitterId:  return o.emitterValid ? QString::number(o.emitterId) : kNA;
    case ColBearing:    return o.bearingValid ? bearingString(o.bearingDeg) : kNA;
        // case ColRange:      return o.rangeValid ? rangeString(o.rangeKm) : kNA;
        // case ColFrequency:  return o.emitterValid ? QString::number(o.frequencyMHz) + QStringLiteral(" MHz") : kNA;

    // Units live in the header tooltip so the columns can stay narrow.
    case ColRange:      return o.rangeValid ? QString::number(o.rangeKm, 'f', 1) : kNA;
    // case ColFrequency:  return o.emitterValid ? QString::number(o.frequencyMHz) : kNA;
    case ColFrequency:  return o.emitterValid ? QString::number(o.frequencyMHz, 'f', 1) : kNA;

    case ColStatus:
        if (!o.rangeValid || rangeKm_ <= 0.0) return kNA;
        return o.inRange ? QStringLiteral("In range") : QStringLiteral("Beyond range");
        // Not produced by the current ESM implementation:
        // case ColBandwidth:
        // case ColPower:
        // case ColSnr:
        // case ColType:
        // case ColLastSeen:

        //Vansh

    case ColBandwidth:  return o.rfValid ? QString::number(o.bandwidthMHz, 'f', 2) : kNA;
    case ColPower:      return o.rfValid ? QString::number(o.rxPowerDbm, 'f', 1) : kNA;
    case ColSnr:        return o.rfValid ? QString::number(o.snrDb, 'f', 1) : kNA;
    case ColType:       return o.type.isEmpty() ? kNA : o.type;
    case ColLastSeen:   return kNA;
    default:
        return kNA;
    }
}

QVariant ESMObservationModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= rows_.size())
        return QVariant();

    const ESMObservation& o = rows_.at(index.row());
    switch (role) {
    case Qt::DisplayRole:
        return cellText(o, index.row(), index.column());
    case Qt::TextAlignmentRole:
        switch (index.column()) {
        case ColPlatformId:
        case ColName:
        case ColType:
        case ColStatus:
            return static_cast<int>(Qt::AlignLeft | Qt::AlignVCenter);
        default:
            return static_cast<int>(Qt::AlignRight | Qt::AlignVCenter);
        }
    case Qt::ForegroundRole:
        if (cellText(o, index.row(), index.column()) == kNA)
            return QBrush(QColor(80, 112, 86));
        return QVariant();
        // case Qt::ToolTipRole:
        //     if (cellText(o, index.row(), index.column()) == kNA)
        //         return QStringLiteral("Not provided by the current ESM data");
        //     return QVariant();
        // default:
        //     return QVariant();
        // }

        //     case Qt::ToolTipRole: {
        //         const QString text = cellText(o, index.row(), index.column());
        //         if (text == kNA)
        //             return QStringLiteral("Not provided by the current ESM data");
        //         if (index.column() == ColRange)
        //             return rangeString(o.rangeKm);                                   // "75.2 km"
        //         if (index.column() == ColFrequency)
        //             return QString::number(o.frequencyMHz) + QStringLiteral(" MHz");
        //         return text;   // full (un-elided) Platform ID / Name / Status etc.
        //     }
        // }

    case Qt::ToolTipRole: {
        const QString text = cellText(o, index.row(), index.column());
        if (text == kNA)
            return QStringLiteral("Not provided by the current ESM data");
        if (index.column() == ColRange)
            return rangeString(o.rangeKm);
        if (index.column() == ColFrequency)
            return QString::number(o.frequencyMHz) + QStringLiteral(" MHz");
        return text;
    }                       // <-- ye ToolTipRole block ki closing
    default:
        return QVariant();
    }                       // <-- ye switch ki closing
}                           // <-- ye data() function ki closing  (shayad yahi missing hai)

// QVariant ESMObservationModel::headerData(int section, Qt::Orientation orientation, int role) const
// {
//     if (orientation != Qt::Horizontal || role != Qt::DisplayRole)
//         return QVariant();

//     switch (section) {
//     case ColIndex:      return QStringLiteral("#");
//     case ColPlatformId: return QStringLiteral("Platform ID");
//     case ColName:       return QStringLiteral("Name");
//     case ColEmitterId:  return QStringLiteral("Emitter ID");
//     case ColBearing:    return QStringLiteral("Bearing");
//     case ColRange:      return QStringLiteral("Range");
//     case ColFrequency:  return QStringLiteral("Frequency");
//     case ColBandwidth:  return QStringLiteral("Bandwidth");
//     case ColPower:      return QStringLiteral("Rx Power");
//     case ColSnr:        return QStringLiteral("SNR");
//     case ColType:       return QStringLiteral("Type");
//     case ColStatus:     return QStringLiteral("Status");
//     case ColLastSeen:   return QStringLiteral("Last Seen");
//     default:            return QVariant();
//     }
// }

QVariant ESMObservationModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation != Qt::Horizontal
        || (role != Qt::DisplayRole && role != Qt::ToolTipRole))
        return QVariant();

    struct H { const char* shortText; const char* fullText; };
    static const H kHeaders[ColumnCount] = {
        { "#",    "Serial number" },
        { "PID",  "Platform ID" },
        { "Name", "Platform name" },
        { "Emt",  "Emitter ID" },
        { "Brg",  "Bearing (degrees, clockwise from north)" },
        { "Rng",  "Range (km)" },
        { "Freq", "Frequency (MHz)" },
        { "BW",   "Bandwidth" },
        { "Rx",   "Received power" },
        { "SNR",  "Signal-to-noise ratio" },
        { "Type", "Emitter type" },
        { "Status", "Detection status (in range / beyond range)" },
        { "Seen", "Last seen" }
    };
    if (section < 0 || section >= ColumnCount)
        return QVariant();
    return QString::fromLatin1(role == Qt::DisplayRole ? kHeaders[section].shortText
                                                       : kHeaders[section].fullText);
}

bool ESMObservationModel::refresh(const ESM* sensor)
{
    QVector<ESMObservation> next;
    SensorState nextState = SensorState::NoSensor;
    double nextRange = 0.0;

    if (sensor) {
        nextState = sensor->Active ? SensorState::Active : SensorState::Inactive;
        nextRange = static_cast<double>(sensor->range);

        next.reserve(sensor->detect.size());
        int i = 0;
        for (const ESMTarget& t : sensor->detect) {
            ++i;
            if (!t.entity)
                continue;   // cannot identify the detection without a platform

            ESMObservation o;
            // o.key = QString::fromStdString(t.entity->ID);
            // if (o.key.isEmpty())
            //     o.key = QString("#%1").arg(i);
            // o.name = QString::fromStdString(t.entity->Name);

            //Vansh
            o.platformId = QString::fromStdString(t.entity->ID);
            o.name = QString::fromStdString(t.entity->Name);

            // One row per emitting sensor, even when multiple emitters
            // belong to the same platform.
            o.key = o.platformId
                    + QStringLiteral("|")
                    + t.emitterSensorId;

            if (t.emitterSensorId.isEmpty()) {
                // Defensive fallback. Normal RF detections should have an emitter ID.
                o.key = o.platformId
                        + QStringLiteral("|")
                        + QString::number(t.emitterid);
            }

            if (o.key == QStringLiteral("|"))
                o.key = QStringLiteral("#%1").arg(i);

            // scan() stores atan2(dLon, dLat) in (-180, 180]; normalise to 0..360.
            if (std::isfinite(t.angle)) {
                double b = std::fmod(t.angle, 360.0);
                if (b < 0.0) b += 360.0;
                if (b >= 360.0) b -= 360.0;
                o.bearingDeg = b;
                o.bearingValid = true;
            }
            if (std::isfinite(t.radius) && t.radius >= 0.0) {
                o.rangeKm = t.radius;
                o.rangeValid = true;
                o.inRange = nextRange > 0.0 && t.radius <= nextRange;
            }
            // if (t.signalValid) {
            //     o.emitterValid = true;
            //     o.emitterId = t.emitterid;
            //     o.frequencyMHz = t.frequency;
            // }

            //Vansh

            if (t.emitterValid) {
                o.emitterValid = true;
                o.emitterId    = t.emitterid;
                o.frequencyMHz = t.frequencyMHz;
                o.type         = t.emitterType;
            }
            if (t.rfValid) {
                o.rfValid      = true;
                o.bandwidthMHz = t.bandwidthHz / 1.0e6;
                o.rxPowerDbm   = t.rxPowerDbm;
                o.snrDb        = t.snrDb;
            }
            next.append(o);
        }
    }

    if (next == rows_ && nextState == state_ && nextRange == rangeKm_)
        return false;

    bool sameKeys = (next.size() == rows_.size());
    for (int i = 0; sameKeys && i < next.size(); ++i)
        sameKeys = (next.at(i).key == rows_.at(i).key);

    if (sameKeys) {
        rows_ = next;
        state_ = nextState;
        rangeKm_ = nextRange;
        if (!rows_.isEmpty())
            emit dataChanged(index(0, 0), index(rows_.size() - 1, ColumnCount - 1));
    } else {
        beginResetModel();
        rows_ = next;
        state_ = nextState;
        rangeKm_ = nextRange;
        endResetModel();
    }
    emit observationsChanged();
    return true;
}

bool ESMObservationModel::clear()
{
    return refresh(nullptr);
}

int ESMObservationModel::rowForKey(const QString& key) const
{
    if (key.isEmpty())
        return -1;
    for (int i = 0; i < rows_.size(); ++i)
        if (rows_.at(i).key == key)
            return i;
    return -1;
}

QString ESMObservationModel::keyAt(int row) const
{
    return (row >= 0 && row < rows_.size()) ? rows_.at(row).key : QString();
}

QString ESMObservationModel::detailText(int row) const
{
    if (row < 0 || row >= rows_.size())
        return QString();
    const ESMObservation& o = rows_.at(row);

    QStringList lines;
    // lines << QStringLiteral("Platform ID: ") + o.key;

    lines << QStringLiteral("Platform ID: ") + o.platformId;
    if (o.emitterValid) {
        lines << QStringLiteral("Emitter ID: ")
        + QString::number(o.emitterId);

        lines << QStringLiteral("Emitter sensor: ") + o.key.section('|', 1);

        lines << QStringLiteral("Frequency: ")
                     + QString::number(o.frequencyMHz)
                     + QStringLiteral(" MHz");
    }
    if (!o.name.isEmpty())
        lines << QStringLiteral("Name: ") + o.name;
    if (o.bearingValid)
        lines << QStringLiteral("Bearing: ") + bearingString(o.bearingDeg);
    if (o.rangeValid)
        lines << QStringLiteral("Range: ") + rangeString(o.rangeKm);
    if (o.emitterValid) {
        lines << QStringLiteral("Emitter ID: ") + QString::number(o.emitterId);
        lines << QStringLiteral("Frequency: ") + QString::number(o.frequencyMHz) + QStringLiteral(" MHz");
    }
    lines << QStringLiteral("Status: ") + cellText(o, row, ColStatus);
    return lines.join(QLatin1Char('\n'));
}

QString ESMObservationModel::summaryText(int row) const
{
    if (row < 0 || row >= rows_.size())
        return QString();
    const ESMObservation& o = rows_.at(row);

    QString s = o.name.isEmpty() ? o.key : o.name;
    if (o.bearingValid)
        s += QStringLiteral("   BRG ") + bearingString(o.bearingDeg);
    if (o.rangeValid)
        s += QStringLiteral("   RNG ") + rangeString(o.rangeKm);
    if (o.emitterValid)
        s += QStringLiteral("   FREQ ") + QString::number(o.frequencyMHz) + QStringLiteral(" MHz");
    return s;
}

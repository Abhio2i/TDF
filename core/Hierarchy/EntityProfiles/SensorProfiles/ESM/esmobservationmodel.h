//============================================================================
// FILE:    esmobservationmodel.h
// MODULE:  ESM (Electronic Support Measures) Display
// PROJECT: Indigenous Scenario and Sensor Simulation Toolkit (ISSST)
//
// DESCRIPTION: Single shared observation source for the ESM panel.
//   - Holds a lightweight snapshot of ESM::detect (no Platform pointers are
//     kept, so a removed platform can never be dereferenced while painting).
//   - Is the QAbstractTableModel behind the observation table AND the data
//     source read by the PPI widget, so both always show the same rows.
//   - Values that the current ESM implementation does not measure are
//     shown as "--" (never as 0).
//============================================================================
#ifndef ESMOBSERVATIONMODEL_H
#define ESMOBSERVATIONMODEL_H

#include "core/Hierarchy/EntityProfiles/SensorProfiles/esm.h"
#include <QAbstractTableModel>
#include <QString>
#include <QVector>

//Vansh

// struct ESMObservation {
//     QString key, name;
//     double  bearingDeg = 0.0, rangeKm = 0.0;
//     bool    bearingValid = false, rangeValid = false, inRange = false;
//     bool    emitterValid = false;        // nominal emitter data (not measured)
//     quint32 emitterId = 0;
//     double  frequencyMHz = 0.0;
//     QString type;
//     bool    rfValid = false;             // simulated link-budget result
//     double  bandwidthMHz = 0.0, rxPowerDbm = 0.0, snrDb = 0.0;

//     bool operator==(const ESMObservation& o) const {
//         return key == o.key && name == o.name && bearingDeg == o.bearingDeg
//                && rangeKm == o.rangeKm && bearingValid == o.bearingValid
//                && rangeValid == o.rangeValid && inRange == o.inRange
//                && emitterValid == o.emitterValid && emitterId == o.emitterId
//                && frequencyMHz == o.frequencyMHz && type == o.type
//                && rfValid == o.rfValid && bandwidthMHz == o.bandwidthMHz
//                && rxPowerDbm == o.rxPowerDbm && snrDb == o.snrDb;
//     }
// };

//Vansh
struct ESMObservation {
    QString key;          // Unique observation identity
    QString platformId;   // Host platform identity
    QString name;

    double bearingDeg = 0.0;
    double rangeKm = 0.0;

    bool bearingValid = false;
    bool rangeValid = false;
    bool inRange = false;

    bool emitterValid = false;
    quint32 emitterId = 0;
    double frequencyMHz = 0.0;
    QString type;

    bool rfValid = false;
    double bandwidthMHz = 0.0;
    double rxPowerDbm = 0.0;
    double snrDb = 0.0;

    bool operator==(const ESMObservation& o) const {
        return key == o.key
               && platformId == o.platformId
               && name == o.name
               && bearingDeg == o.bearingDeg
               && rangeKm == o.rangeKm
               && bearingValid == o.bearingValid
               && rangeValid == o.rangeValid
               && inRange == o.inRange
               && emitterValid == o.emitterValid
               && emitterId == o.emitterId
               && frequencyMHz == o.frequencyMHz
               && type == o.type
               && rfValid == o.rfValid
               && bandwidthMHz == o.bandwidthMHz
               && rxPowerDbm == o.rxPowerDbm
               && snrDb == o.snrDb;
    }
};

class ESMObservationModel : public QAbstractTableModel
{
    Q_OBJECT
public:
    enum Column {
        ColIndex = 0,
        ColPlatformId,
        ColName,
        ColEmitterId,
        ColBearing,
        ColRange,
        ColFrequency,
        ColBandwidth,
        ColPower,
        ColSnr,
        ColType,
        ColStatus,
        ColLastSeen,
        ColumnCount
    };
    enum class SensorState { NoSensor, Inactive, Active };

    explicit ESMObservationModel(QObject* parent = nullptr);

    // QAbstractTableModel
    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    int columnCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation,
                        int role = Qt::DisplayRole) const override;

    /* Re-read sensor->detect. Returns true if anything visible changed.
     * Rows are only reset when the set/order of keys changes, otherwise only
     * dataChanged is emitted, so the table selection and scroll position
     * survive the normal per-tick updates. */
    bool refresh(const ESM* sensor);
    /* Drop all rows (no sensor). */
    bool clear();

    const QVector<ESMObservation>& observations() const { return rows_; }
    int rowForKey(const QString& key) const;
    QString keyAt(int row) const;

    SensorState sensorState() const { return state_; }
    double rangeKm() const { return rangeKm_; }

    /* Multi-line text with the values that are actually available. */
    QString detailText(int row) const;
    /* One-line text for the spectrum header. */
    QString summaryText(int row) const;

signals:
    /* Emitted after refresh()/clear() when something changed. */
    void observationsChanged();

private:
    QString cellText(const ESMObservation& o, int row, int column) const;

    QVector<ESMObservation> rows_;
    SensorState state_ = SensorState::NoSensor;
    double rangeKm_ = 0.0;
};

#endif // ESMOBSERVATIONMODEL_H
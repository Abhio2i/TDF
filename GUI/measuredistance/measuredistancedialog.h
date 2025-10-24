#ifndef MEASUREDISTANCEDIALOG_H
#define MEASUREDISTANCEDIALOG_H
#include <QDialog>
#include <QListWidget>
#include <QLineEdit>
#include <QRadioButton>
#include <QComboBox>
class MeasureDistanceDialog : public QDialog
{
    Q_OBJECT
public:
    explicit MeasureDistanceDialog(QWidget *parent = nullptr);
    void addMeasurement(double x, double y, double distance);
    void clearMeasurements();
    bool isEllipsoidal() const;
    double getCurrentConversionFactor() const;
    QString getCurrentUnitString() const;
signals:
    void measurementTypeChanged(bool isEllipsoidal);
    void newMeasurementRequested();
    void unitChanged(const QString &unit);
private:
    QListWidget *segmentsList;
    QLineEdit *totalDistanceEdit;
    QRadioButton *cartesianRadio;
    QRadioButton *ellipsoidalRadio;
    QComboBox *unitComboBox;
    struct Measurement {
        double x;
        double y;
        double distance;
    };
    QList<Measurement> measurements;
    enum Unit { Meters, Kilometers, Feet, Miles, Degrees };
    Unit currentUnit = Meters;
    double getConversionFactor(Unit unit) const;
    QString getUnitString(Unit unit) const;
    void updateTotalDistance();
private slots:
    void updateMeasurementDisplay();
    void onMeasurementTypeChanged();
    void onNewButtonClicked();
    void onCopyButtonClicked();
    void onInfoButtonClicked();
    void onConfigButtonClicked();
    void onUnitChanged(int index);
};
#endif // MEASUREDISTANCEDIALOG_H

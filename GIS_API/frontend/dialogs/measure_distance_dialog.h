#ifndef API_MEASURE_DISTANCE_DIALOG_H
#define API_MEASURE_DISTANCE_DIALOG_H

#include <QDialog>
#include <QTableWidget>
#include <QComboBox>
#include <QLabel>
#include <QPushButton>
#include <QPointF>
#include <QVector>

class ApiMeasureDistanceDialog : public QDialog {
    Q_OBJECT
public:
    explicit ApiMeasureDistanceDialog(QWidget* parent = nullptr);
    ~ApiMeasureDistanceDialog() override;
    void setPoints(const QVector<QPointF>& geoPoints, const QVector<double>& segmentMeters);
    void clearMeasurement();

signals:
    void requestNew();
    void closedByUser();

protected:
    void closeEvent(QCloseEvent* event) override;

private slots:
    void onUnitChanged(int index);
    void onNew();
    void onCopy();
    void onClose();

private:
    void setupUi();
    void rebuildTable();
    double convertFromMeters(double meters) const;
    QString unitSuffix() const;

    QTableWidget* m_table = nullptr;
    QComboBox* m_unitCombo = nullptr;
    QLabel* m_totalLabel = nullptr;
    QPushButton* m_newBtn = nullptr;
    QPushButton* m_copyBtn = nullptr;
    QPushButton* m_closeBtn = nullptr;

    QVector<QPointF> m_points;
    QVector<double> m_segmentMeters;
    int m_unitIndex = 0;
};

#endif

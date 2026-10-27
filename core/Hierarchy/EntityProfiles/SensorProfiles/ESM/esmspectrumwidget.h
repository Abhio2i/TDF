//============================================================================
// FILE:    esmspectrumwidget.h
// MODULE:  ESM (Electronic Support Measures) Display
// PROJECT: Indigenous Scenario and Sensor Simulation Toolkit (ISSST)
//
// DESCRIPTION: Signal Spectrum graph (PSD [dBm/Hz] versus frequency [MHz]).
//
//   The current ESM implementation produces NO frequency-resolved signal
//   data, so by default this widget shows the graph layout with the message
//   "Spectrum data unavailable". Nothing is synthesised from the emitter list.
//
//   Future ESM processing modules supply real samples through setSpectrum().
//============================================================================
#ifndef ESMSPECTRUMWIDGET_H
#define ESMSPECTRUMWIDGET_H

#include <QString>
#include <QVector>
#include <QWidget>

class ESMSpectrumWidget : public QWidget
{
    Q_OBJECT
public:
    explicit ESMSpectrumWidget(QWidget* parent = nullptr);

    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;

    /* Supply a measured spectrum.
     *   frequenciesMHz : x values in MHz (same length as psdDbmPerHz, >= 2)
     *   psdDbmPerHz    : power spectral density in dBm/Hz
     * Invalid input (size mismatch, < 2 points, NaN/Inf) clears the display. */
    void setSpectrum(const QVector<double>& frequenciesMHz,
                     const QVector<double>& psdDbmPerHz);
    void clearSpectrum();
    bool hasSpectrum() const { return !freqMHz_.isEmpty(); }

    /* Optional vertical marker (e.g. selected emitter frequency). */
    void setMarkerFrequencyMHz(double mhz);
    void clearMarker();

    /* One-line text describing the selected observation (header). */
    void setSelectedInfo(const QString& text);

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    QVector<double> freqMHz_;
    QVector<double> psd_;
    bool hasMarker_ = false;
    double markerMHz_ = 0.0;
    QString selectedInfo_;
};

#endif // ESMSPECTRUMWIDGET_H

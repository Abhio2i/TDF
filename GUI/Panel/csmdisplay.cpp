// //============================================================================
// // FILE:         csmdisplay.cpp
// // MODULE:       CSM (Communications Support Measures) Display
// // PROJECT:      Indigenous Scenario and Sensor Simulation Toolkit (ISSST)
// // ORGANISATION: Oxygen 2 Innovation (O2I).
// // STANDARD:     RTCA DO-178C / ED-12C, DAL B
// // COVERAGE:     Branch / Decision Coverage required (100% true/false paths)
// //
// // DESCRIPTION:  Implements the CSMDisplay class: a frequency-spectrum frame
// //               (modelled PSD trace from CSM::lastSpectrum()) above
// //               an observations table fed from the selected CSM sensor's
// //               per-radio scan result (CSM::lastScanResult()). See
// //               csmdisplay.h for the data policy.
// //
// // REQUIREMENTS: Implements REQ-CSM-010, REQ-CSM-015, REQ-CSM-016, REQ-CSM-017
// //
// // AUTHOR:       Arti Rajpoot
// // REVIEWED BY:  [Reviewer Name], [Review Date] — SPR-CSM-001
// //
// //
// // COPYRIGHT:    Oxygen 2 Innovation (O2I). All rights reserved.
// //               Restricted circulation — defence simulation use only.
// //============================================================================

// #include "csmdisplay.h"                            // For CSM display class
// #include "core/Hierarchy/Utils/entityutils.h"
// #include "core/Hierarchy/EntityProfiles/SensorProfiles/csm.h"   // Per-radio scan result
// #include "qelapsedtimer.h"
// #include <QPainter>                                // For painting operations
// #include <QPaintEvent>                             // For paint events
// #include <QMouseEvent>                             // For double-click
// #include <QPolygonF>                              // For spectrum trace
// #include <QWheelEvent>                             // For table scrolling
// #include <QFont>                                   // For font settings
// #include <QFontMetrics>                            // For text layout
// #include <QtMath>                                  // For math functions
// #include <QDebug>                                  // For debug output
// #include <cmath>
// #include <core/Debug/console.h>                    // For console error logging

// namespace {

// const int kColumnCount = 8;
// // Column width fractions (sum = 1.0)
// const double kColumnFraction[kColumnCount] = { 0.05, 0.15, 0.12, 0.14, 0.12, 0.09, 0.15, 0.18 };
// const char* const kColumnTitle[kColumnCount] = {
//     "#", "FREQUENCY", "BANDWIDTH", "MOD (CFG)", "LEVEL", "SNR", "RNG / BRG", "EMITTER"
// };
// // const char* const kColumnTitle[kColumnCount] = {
// //     "#", "FREQUENCY", "BANDWIDTH", "MOD (CFG)", "LEVEL", "EMITTER"
// // };
// const char* const kPlaceholder = "--";

// QString formatFrequencyHz(double hz)
// {
//     if (!(hz > 0.0) || !std::isfinite(hz)) return kPlaceholder;
//     if (hz >= 1e9) return QString::number(hz / 1e9, 'f', 3) + " GHz";
//     if (hz >= 1e6) return QString::number(hz / 1e6, 'f', 3) + " MHz";
//     return QString::number(hz / 1e3, 'f', 1) + " kHz";
// }

// QString formatBandwidthHz(double hz)
// {
//     if (!(hz > 0.0) || !std::isfinite(hz)) return kPlaceholder;
//     if (hz >= 1e6) return QString::number(hz / 1e6, 'f', 2) + " MHz";
//     if (hz >= 1e3) return QString::number(hz / 1e3, 'f', 1) + " kHz";
//     return QString::number(hz, 'f', 0) + " Hz";
// }

// QString formatSnr(const csm::Quantity& q)
// {
//     // Calculated SNR over the signal's occupied bandwidth (model result).
//     if (!q.valid()) return kPlaceholder;
//     return QString::number(q.value, 'f', 1) + " dB";
// }

// QString formatRangeBearing(const csm::Quantity& range, const csm::Quantity& bearing)
// {
//     // Calculated geometry: great-circle range [km] and bearing in the
//     // observer's local frame [deg, 0-360].
//     if (!range.valid() || !bearing.valid()) return kPlaceholder;
//     double b = std::fmod(bearing.value, 360.0);
//     if (b < 0.0) b += 360.0;
//     return QString("%1 km / %2%3")
//         .arg(range.value / 1000.0, 0, 'f', 1)
//         .arg(b, 0, 'f', 0)
//         .arg(QChar(0x00B0));
// }


// QString formatLevel(const csm::Quantity& q)
// {
//     // Calculated link-budget level (model result, not a measurement).
//     if (!q.valid()) return kPlaceholder;
//     return QString::number(q.value, 'f', 1) + " dBm";
// }

// } // namespace

// // %%% Constructor %%%
// /* Initialize CSM display */
// CSMDisplay::CSMDisplay(QWidget *parent)
//     : QWidget(parent)
// {
//     // Set background color
//     setStyleSheet("background-color: black;");
//     // Set size policy with aspect ratio
//     QSizePolicy policy(QSizePolicy::Preferred, QSizePolicy::Preferred);
//     policy.setHeightForWidth(true);
//     setSizePolicy(policy);
//     padding = 6;
//     sensorDropdown = new QComboBox(this);
//     sensorDropdown->setStyleSheet(
//         "QComboBox { background-color: #001a00; color: #00ff00; "
//         "border: 1px solid #00ff00; font-size: 10px; padding: 2px; }"
//         "QComboBox QAbstractItemView { background-color: #001a00; "
//         "color: #00ff00; selection-background-color: #003300; }"
//         );
//     sensorDropdown->hide();
//     connect(sensorDropdown, QOverload<int>::of(&QComboBox::currentIndexChanged),
//             this, &CSMDisplay::onSensorSelected);
// }

// // %%% Size Management %%%
// /* Provide size hint for widget */
// QSize CSMDisplay::sizeHint() const
// {
//     int defaultWidth = 400;
//     return QSize(defaultWidth, heightForWidth(defaultWidth));
// }

// /* Provide minimum size for widget */
// QSize CSMDisplay::minimumSize() const
// {
//     int minW = 250;
//     return QSize(minW, heightForWidth(minW));
// }

// /* Calculate height based on width and aspect ratio */
// int CSMDisplay::heightForWidth(int width) const
// {
//     return qRound(width * ASPECT_RATIO);
// }

// /* Mouse move: no hover behaviour in the spectrum layout */
// void CSMDisplay::mouseMoveEvent(QMouseEvent *event)
// {
//     QWidget::mouseMoveEvent(event);
// }

// /* Double-click: reset wheel zoom or toggle strongest-signal zoom */
// void CSMDisplay::mouseDoubleClickEvent(QMouseEvent *event)
// {
//     if (viewZoomed_) {
//         viewZoomed_ = false;
//         update();
//         event->accept();
//         return;
//     }

//     if (spectrum_ && spectrum_->available && spectrum_->hasZoom) {
//         zoomView_ = !zoomView_;
//         update();
//         event->accept();
//         return;
//     }

//     QWidget::mouseDoubleClickEvent(event);
// }

// /* Handle mouse leave events */
// void CSMDisplay::leaveEvent(QEvent *event)
// {
//     QWidget::leaveEvent(event);
// }

// /* Scroll the observations table */
// /* Spectrum: mouse-wheel zoom. Observations table: normal row scrolling. */
// void CSMDisplay::wheelEvent(QWheelEvent *event)
// {
//     const double notches = event->angleDelta().y() / 120.0;

//     // Qt 5 compatible mouse position.
//     const QPoint pos = event->pos();

//     // If the mouse is over the spectrum plot, zoom its frequency range.
//     if (notches != 0.0 &&
//         spectrum_ &&
//         spectrum_->available &&
//         spectrumPlot_.isValid() &&
//         spectrumPlot_.contains(pos)) {

//         if (applyWheelZoom(qBound(-10.0, notches, 10.0), pos.x())) {
//             update();
//         }

//         event->accept();
//         return;
//     }

//     // Otherwise retain the existing observations-table scrolling.
//     const int maxScroll = qMax(0, rows.size() - visibleRows);
//     const int steps = event->angleDelta().y() / 120;

//     if (maxScroll == 0 || steps == 0) {
//         event->ignore();
//         return;
//     }

//     const int next = qBound(0, rowScroll - steps, maxScroll);

//     if (next != rowScroll) {
//         rowScroll = next;
//         update();
//     }

//     event->accept();
// }


// /* Apply wheel zoom while keeping the frequency under the cursor fixed. */
// // bool CSMDisplay::applyWheelZoom(double notches, int xPixel)
// // {
// //     if (!spectrum_ ||
// //         !spectrum_->available ||
// //         spectrumPlot_.width() < 2) {
// //         return false;
// //     }

// //     const double fullLo = spectrum_->span.startHz;
// //     const double fullHi = spectrum_->span.stopHz();
// //     const double fullW = fullHi - fullLo;

// //     if (!std::isfinite(fullW) || !(fullW > 0.0)) {
// //         return false;
// //     }

// //     double lo = fullLo;
// //     double hi = fullHi;

// //     if (viewZoomed_) {
// //         lo = qMax(fullLo, viewLoHz_);
// //         hi = qMin(fullHi, viewHiHz_);

// //         if (!(hi > lo)) {
// //             lo = fullLo;
// //             hi = fullHi;
// //         }
// //     }

// //     const double w = hi - lo;

// //     const double t = qBound(
// //         0.0,
// //         double(xPixel - spectrumPlot_.left()) / spectrumPlot_.width(),
// //         1.0
// //         );

// //     // Frequency beneath the cursor remains the zoom anchor.
// //     const double anchor = lo + t * w;

// //     const double newW = qBound(
// //         qMin(kMinViewHz, fullW),
// //         w * std::pow(kWheelStep, notches),
// //         fullW
// //         );

// //     // Reaching the full range resets wheel zoom.
// //     if (newW >= fullW * (1.0 - 1e-9)) {
// //         const bool wasZoomed = viewZoomed_;
// //         viewZoomed_ = false;
// //         return wasZoomed;
// //     }

// //     const double newLo = qBound(
// //         fullLo,
// //         anchor - t * newW,
// //         fullHi - newW
// //         );

// //     viewLoHz_ = newLo;
// //     viewHiHz_ = newLo + newW;
// //     viewZoomed_ = true;

// //     return true;
// // }
// /* Apply wheel zoom while keeping the frequency under the cursor fixed. */
// bool CSMDisplay::applyWheelZoom(double notches, int xPixel)
// {
//     if (!spectrum_ ||
//         !spectrum_->available ||
//         spectrumPlot_.width() < 2) {
//         return false;
//     }

//     const double fullLo = spectrum_->span.startHz;
//     const double fullHi = spectrum_->span.stopHz();
//     const double fullW = fullHi - fullLo;

//     if (!std::isfinite(fullW) || !(fullW > 0.0)) {
//         return false;
//     }

//     double lo = fullLo;
//     double hi = fullHi;

//     if (viewZoomed_) {
//         lo = qMax(fullLo, viewLoHz_);
//         hi = qMin(fullHi, viewHiHz_);

//         if (!(hi > lo)) {
//             lo = fullLo;
//             hi = fullHi;
//         }
//     }

//     const double w = hi - lo;

//     const double t = qBound(
//         0.0,
//         double(xPixel - spectrumPlot_.left()) / spectrumPlot_.width(),
//         1.0
//         );

//     // Frequency beneath the cursor remains the zoom anchor.
//     const double anchor = lo + t * w;

//     const double newW = qBound(
//         qMin(kMinViewHz, fullW),
//         w * std::pow(kWheelStep, notches),
//         fullW
//         );

//     // Reaching the full range resets wheel zoom.
//     if (newW >= fullW * (1.0 - 1e-9)) {
//         const bool wasZoomed = viewZoomed_;
//         viewZoomed_ = false;
//         return wasZoomed;
//     }

//     const double newLo = qBound(
//         fullLo,
//         anchor - t * newW,
//         fullHi - newW
//         );

//     viewLoHz_ = newLo;
//     viewHiHz_ = newLo + newW;
//     viewZoomed_ = true;

//     return true;
// }


// /* Return the valid wheel-zoom window, clipped to the monitor span. */
// bool CSMDisplay::effectiveView(
//     const csm::spectrum::Result &res,
//     double &lo,
//     double &hi
//     ) const
// {
//     if (!viewZoomed_) {
//         return false;
//     }

//     const double fullW = res.span.stopHz() - res.span.startHz;

//     lo = qMax(res.span.startHz, viewLoHz_);
//     hi = qMin(res.span.stopHz(), viewHiHz_);

//     return std::isfinite(lo) &&
//            std::isfinite(hi) &&
//            hi > lo &&
//            (hi - lo) >= qMin(kMinViewHz, fullW) * 0.999;
// }



// /* Select entity for display */
// void CSMDisplay::selectEntity(Entity* entit)
// {
//     entity = nullptr;
//     id = "";
//     rowScroll = 0;
//     zoomView_ = false;
//     viewZoomed_ = false;
//     scanObserved = false;   // new selection: no scan seen yet

//     Platform* platform = dynamic_cast<Platform*>(entit);
//     if (!platform) {
//         Console::error("Entity is not a Platform");
//         setWindowTitle("CSM Display (No Platform)");
//         refreshObservations(false);
//         update();
//         return;
//     }

//     // Set entity ID and pointer
//     id = QString::fromStdString(platform->ID);
//     entity = platform;

//     sensor = nullptr;
//     sensorlist.clear();
//     for (auto const& pair :  entity->sensors.sensors) {
//         Sensor* s = pair.second;
//         if (s && s->subType == Sensor::SubType::CSM) {
//             if(sensor == nullptr){
//                 sensor = s;
//             }
//             sensorlist.append(s);
//             setWindowTitle("CSM Display (" + QString::fromStdString(entity->Name) + ")");
//         }
//     }

//     updateDropdown();
//     refreshObservations(false);
//     update();
// }

// /* Remove entity from display */
// void CSMDisplay::RemoveEntity(QString ID)
// {
//     if (id == ID) {
//         entity = nullptr;
//         sensor = nullptr;

//         sensorlist.clear();
//         if (sensorDropdown) {
//             QSignalBlocker blocker(sensorDropdown);
//             sensorDropdown->clear();
//             sensorDropdown->hide();
//         }

//         id.clear();
//         setWindowTitle("CSM Display");
//         rowScroll = 0;
//         refreshObservations(false);

//         update();
//     }
// }

// /* Update display with new data (called on every simulation update) */
// void CSMDisplay::updateRadar()
// {
//     if (entity && sensor) {
//         bool stillValid = false;
//         for (auto const& pair : entity->sensors.sensors) {
//             if (pair.second == sensor) { stillValid = true; break; }
//         }
//         if (!stillValid) sensor = nullptr;
//     }

//     if (entity && sensor) {
//         setRange(sensor->range);
//         if (refreshObservations(true)) update();
//     } else {
//         if (refreshObservations(false)) update();
//     }
// }

// // %%% Data Methods %%%
// /* Rebuild cached rows and state from the selected sensor.
//  * scanTick is true only when called from the periodic update, so that
//  * selecting a sensor does not by itself count as a completed scan. */
// bool CSMDisplay::refreshObservations(bool scanTick)
// {
//     const bool newActive = (entity && sensor && sensor->Active);

//     bool newScan = scanObserved;
//     if (!newActive)      newScan = false;
//     else if (scanTick)   newScan = true;

//     QVector<QStringList> newRows;
//     bool configured = false;
//     bool calculated = false;
//     const CSM* csmSensor = newActive ? dynamic_cast<const CSM*>(sensor) : nullptr;
//     // Cached modelled spectrum (shared pointer: no copy, changes only on recompute).
//     const std::shared_ptr<const csm::spectrum::Result> newSpectrum =
//         csmSensor ? csmSensor->lastSpectrum() : nullptr;
//     const csm::monitor::Window newMonitor =
//         csmSensor ? csmSensor->lastMonitor() : csm::monitor::Window();
//     if (csmSensor) {
//         int n = 0;
//         for (const csm::Observation &o : csmSensor->lastScanResult().observations) {
//             ++n;
//             QString freq = kPlaceholder, bw = kPlaceholder, mod = kPlaceholder;
//             if (o.frequencyHz.valid()) { freq = formatFrequencyHz(o.frequencyHz.value); configured = true; }
//             if (o.bandwidthHz.valid()) { bw = formatBandwidthHz(o.bandwidthHz.value); configured = true; }
//             if (o.modulationSource == csm::Provenance::Configured && !o.modulation.empty()) {
//                 mod = QString::fromStdString(o.modulation);
//                 configured = true;
//             }
//             if (o.levelDbm.valid() || o.snrDb.valid() ||
//                 o.distanceM.valid() || o.bearingDeg.valid()) calculated = true;


//             const QString platformName = QString::fromStdString(
//                 o.platformName.empty() ? o.platformId : o.platformName);
//             const QString radioName = QString::fromStdString(
//                 o.radioName.empty() ? o.radioId : o.radioName);

//             QStringList row;
//             row << QString::number(n) << freq << bw << mod
//                 << formatLevel(o.levelDbm)
//                 << formatSnr(o.snrDb)
//                 << formatRangeBearing(o.distanceM, o.bearingDeg)
//                 << (platformName + "/" + radioName);
//             newRows.append(row);
//         }
//     }

//     const bool changed = (newActive != sensorActive) || (newScan != scanObserved)
//                          || (configured != showsConfiguredValues)
//                          || (calculated != showsCalculatedLevel) || (newRows != rows)
//                          || (newSpectrum != spectrum_)
//                          || (newMonitor.valid != monitor_.valid) || (newMonitor.mode != monitor_.mode)
//                          || (newMonitor.autoRange != monitor_.autoRange)
//                          || (newMonitor.stepIndex != monitor_.stepIndex)
//                          || (newMonitor.centreHz != monitor_.centreHz)
//                          || (newMonitor.spanHz != monitor_.spanHz);


//     // Reset wheel zoom when the monitored frequency window changes.
//     if (viewZoomed_ &&
//         (!newSpectrum ||
//          !newSpectrum->available ||
//          newMonitor.centreHz != monitor_.centreHz ||
//          newMonitor.spanHz != monitor_.spanHz)) {
//         viewZoomed_ = false;
//     }
//     sensorActive = newActive;
//     scanObserved = newScan;
//     showsConfiguredValues = configured;
//     showsCalculatedLevel = calculated;
//     rows = newRows;
//     spectrum_ = newSpectrum;
//     monitor_ = newMonitor;
//     return changed;
// }

// /* Text shown in the empty observations table */
// QString CSMDisplay::observationsMessage() const
// {
//     if (!entity || !sensor) return "No CSM Sensor";
//     if (!sensorActive)      return "CSM Sensor Inactive";
//     if (!scanObserved)      return "Waiting for First Scan";
//     return "No CSM Signals Detected";
// }

// /* Mode text from the actual configuration: MONITOR/SCAN and AUTO/MANUAL range */
// QString CSMDisplay::modeLabel() const
// {
//     return QString("MODE : %1 (%2)")
//     .arg(QString::fromLatin1(csm::monitor::modeName(monitor_.mode)))
//         .arg(monitor_.autoRange ? QStringLiteral("AUTO") : QStringLiteral("MANUAL"));
// }

// /* Monospace display font scaled with widget width */
// QFont CSMDisplay::displayFont(int delta, bool bold) const
// {
//     QFont f("Monospace");
//     f.setStyleHint(QFont::TypeWriter);
//     f.setPixelSize(qMax(7, qBound(9, width() / 46, 15) + delta));
//     f.setBold(bold);
//     return f;
// }

// /* Split the widget into spectrum (top) and observations (bottom) */
// void CSMDisplay::sectionRects(QRect &spectrum, QRect &observations) const
// {
//     const QRect total = rect().adjusted(padding, padding, -padding, -padding);
//     const int specH = qRound(total.height() * 0.70);
//     spectrum = QRect(total.left(), total.top(), total.width(), specH);
//     observations = QRect(total.left(), total.top() + specH + padding,
//                          total.width(), total.height() - specH - padding);
// }

// /* Main paint event handler */
// void CSMDisplay::paintEvent(QPaintEvent * /*event*/)
// {
//     QElapsedTimer timer;
//     timer.start();

//     if (width() <= 0 || height() <= 0) return;

//     QPainter p(this);
//     p.fillRect(rect(), Qt::black);

//     QRect spectrum, observations;
//     sectionRects(spectrum, observations);
//     drawSpectrum(p, spectrum);
//     drawObservations(p, observations);

//     if (Profiler::currentFrame)
//         Profiler::currentFrame->csmdisplay = timer.elapsed();
// }
// void CSMDisplay::drawSpectrum(QPainter &p, const QRect &r)
// {

//     spectrumPlot_ = QRect();
//     if (r.width() < 40 || r.height() < 40) return;

//     p.save();
//     p.setPen(QPen(radarGreen, 1));
//     p.setBrush(Qt::NoBrush);
//     p.drawRect(r);

//     // Header: title (left), monitor-mode indicator (right)
//     const QFont titleFont = displayFont(4, true);
//     const QFontMetrics tfm(titleFont);
//     p.setFont(titleFont);
//     p.drawText(r.left() + 10, r.top() + 4 + tfm.ascent(), "CSM");

//     const QFont f = displayFont();
//     const QFontMetrics fm(f);
//     p.setFont(f);
//     const QString mode = modeLabel();
//     p.drawText(r.right() - 10 - fm.horizontalAdvance(mode),
//                r.top() + 4 + (tfm.height() - fm.height()) / 2 + fm.ascent(), mode);

//     // Plot area
//     const int headerH = tfm.height() + 8;
//     const QRect plot(QPoint(r.left() + fm.height() * 2 + 6, r.top() + headerH + 4),
//                      QPoint(r.right() - 12, r.bottom() - fm.height() - 18));
//     if (plot.width() < 30 || plot.height() < 30) {
//         p.restore();
//         return;
//     }
//     spectrumPlot_ = plot;

//     // Axes
//     p.setPen(QPen(radarGreen, 1));
//     p.drawLine(plot.left(), plot.top(), plot.left(), plot.bottom());
//     p.drawLine(plot.left(), plot.bottom(), plot.right(), plot.bottom());

//     // Y ticks: major (long) at grid lines, minor (short) between them
//     const double step = plot.height() / 5.8;
//     for (int n = 0; n <= 11; ++n) {
//         const int y = qRound(plot.top() + n * 0.5 * step);
//         const int len = (n % 2 == 1) ? 9 : 5;
//         p.drawLine(plot.left() - len, y, plot.left(), y);
//     }
//     // X ticks
//     for (int i = 0; i <= 6; ++i) {
//         const int x = qRound(plot.left() + plot.width() * i / 6.0);
//         p.drawLine(x, plot.bottom(), x, plot.bottom() + 8);
//     }

//     // Horizontal dashed grid lines
//     QPen dash(radarGreen, 1);
//     dash.setStyle(Qt::CustomDashLine);
//     dash.setDashPattern(QVector<qreal>() << 3 << 3);
//     p.setPen(dash);
//     for (int k = 0; k < 6; ++k) {
//         const int y = qRound(plot.top() + (0.5 + k) * step);
//         p.drawLine(plot.left() + 1, y, plot.right() - 6, y);
//     }

//     const bool haveSpectrum = spectrum_ && spectrum_->available;

//     // Axis titles
//     p.setPen(QPen(radarGreen, 1));
//     p.save();
//     p.translate(r.left() + fm.height(), plot.center().y());
//     p.rotate(-90);
//     p.drawText(QRect(-plot.height() / 2, -fm.height() / 2, plot.height(), fm.height()),
//                Qt::AlignCenter, haveSpectrum ? "PSD  dBm/Hz" : "LEVEL");
//     p.restore();

//     if (haveSpectrum) {
//         drawSpectrumTrace(p, plot);
//         p.restore();
//         return;
//     }

//     p.drawText(QRect(plot.left(), plot.bottom() + 10, plot.width(), fm.height() + 4),
//                Qt::AlignCenter, "FREQUENCY");

//     // No valid spectrum: say why (never draw a placeholder trace)
//     QString msg;
//     if (spectrum_)               msg = "Spectrum unavailable: " + QString::fromStdString(spectrum_->reason);
//     else if (!entity || !sensor) msg = "Spectrum unavailable: no CSM sensor";
//     else if (!sensorActive)      msg = "Spectrum unavailable: CSM sensor inactive";
//     else                         msg = "Spectrum unavailable: waiting for first scan";
//     p.setFont(displayFont(1));
//     p.drawText(plot.adjusted(10, 10, -10, -10), Qt::AlignCenter | Qt::TextWordWrap, msg);
//     p.restore();
// }

// /* Draw the modelled PSD trace. All numbers come from the cached
//  * csm::spectrum::Result; nothing is calculated here except pixel positions. */
// void CSMDisplay::drawSpectrumTrace(QPainter &p, const QRect &plot)
// {
//     // const csm::spectrum::Result &res = *spectrum_;
//     // const bool zoom = zoomView_ && res.hasZoom;
//     // const csm::spectrum::Trace &tr = zoom ? res.zoom : res.span;
//     // const int n = static_cast<int>(tr.psdDbmHz.size());
//     // double lo = 0.0, hi = 0.0;
//     // if (n < 2 || !(tr.binHz > 0.0) || !csm::spectrum::axisRange(tr, lo, hi)) return;


//     const csm::spectrum::Result &res = *spectrum_;

//     // Mouse-wheel zoom is evaluated using the same analytical model.
//     csm::spectrum::Trace wheelTrace;
//     double vLo = 0.0;
//     double vHi = 0.0;

//     const bool wheel =
//         effectiveView(res, vLo, vHi) &&
//         csm::spectrum::windowTrace(
//             res,
//             vLo,
//             vHi,
//             csm::spectrum::kDisplayBins,
//             wheelTrace
//             );

//     // Priority: wheel zoom > existing double-click zoom > full spectrum.
//     const bool zoom = !wheel && zoomView_ && res.hasZoom;

//     const csm::spectrum::Trace &tr =
//         wheel ? wheelTrace : (zoom ? res.zoom : res.span);

//     const int n = static_cast<int>(tr.psdDbmHz.size());

//     double lo = 0.0;
//     double hi = 0.0;

//     if (n < 2 ||
//         !(tr.binHz > 0.0) ||
//         !csm::spectrum::axisRange(tr, lo, hi)) {
//         return;
//     }

//     const QFont small = displayFont(-2);
//     const QFontMetrics sfm(small);
//     p.save();
//     p.setFont(small);
//     p.setPen(QPen(radarGreen, 1));

//     // Pixel mapping. The six dashed grid lines (drawn by the caller) sit at
//     // plot.top() + (0.5 + k) * step; grid line 0 = hi dB, grid line 5 = lo dB.
//     const double step = plot.height() / 5.8;
//     const double fStart = tr.startHz, fStop = tr.stopHz();
//     auto xOf = [&](double f) { return plot.left() + (f - fStart) / (fStop - fStart) * plot.width(); };
//     auto yOf = [&](double v) { return plot.top() + 0.5 * step + (hi - v) / (hi - lo) * 5.0 * step; };

//     // Y labels on the grid lines (dBm/Hz)
//     for (int k = 0; k < 6; ++k) {
//         const double v = hi - k * (hi - lo) / 5.0;
//         p.drawText(QPoint(plot.left() + 4, qRound(plot.top() + (0.5 + k) * step) - 2),
//                    QString::number(v, 'f', 0));
//     }

//     // X labels: start / centre / stop of the displayed frequency range
//     const int ly = plot.bottom() + 10;
//     const QRect lr(plot.left(), ly, plot.width(), sfm.height() + 4);
//     p.drawText(lr, Qt::AlignLeft | Qt::AlignVCenter, formatFrequencyHz(fStart));
//     p.drawText(lr, Qt::AlignHCenter | Qt::AlignVCenter, formatFrequencyHz(0.5 * (fStart + fStop)));
//     p.drawText(lr, Qt::AlignRight | Qt::AlignVCenter, formatFrequencyHz(fStop));

//     // Trace (green on black), clipped to the plot
//     p.setClipRect(plot);
//     QPolygonF line;
//     line.reserve(n);
//     for (int i = 0; i < n; ++i)
//         line << QPointF(xOf(tr.binCentreHz(i)), yOf(tr.psdDbmHz[static_cast<size_t>(i)]));
//     p.setPen(QPen(radarGreen, 1));
//     p.drawPolyline(line);

//     // Markers: small triangle on top of the DISPLAYED value at each drawn
//     // emitter's configured frequency. Position only; the true peak PSD is
//     // quoted in the text below, the trace is not altered.
//     for (const csm::spectrum::EmitterNote &e : res.emitters) {
//         if (!e.drawn || e.frequencyHz < fStart || e.frequencyHz >= fStop) continue;
//         const int bi = qBound(0, static_cast<int>((e.frequencyHz - fStart) / tr.binHz), n - 1);
//         const double x = xOf(e.frequencyHz);
//         const double y = yOf(tr.psdDbmHz[static_cast<size_t>(bi)]);
//         QPolygonF tri;
//         tri << QPointF(x - 4, y - 10) << QPointF(x + 4, y - 10) << QPointF(x, y - 3);
//         p.drawPolygon(tri);
//     }
//     p.setClipping(false);

//     // // Provenance and scale notes (top of plot)
//     // int ty = plot.top() + sfm.ascent() + 2;
//     // const int tx = plot.left() + 4;
//     // const int tw = plot.width() - 8;
//     // auto line1 = [&](const QString &t) {
//     //     p.drawText(QPoint(tx, ty), sfm.elidedText(t, Qt::ElideRight, tw));
//     //     ty += sfm.height();
//     // };
//     // line1(QString::fromLatin1(csm::spectrum::provenanceLabel()));
//     // line1(QString("%1  bin %2  noise %3 dBm/Hz (thermal model)%4")
//     //           .arg(wheel ? QStringLiteral("WHEEL-ZOOM")
//     //                      : (zoom ? QStringLiteral("ZOOM") : QStringLiteral("SPAN")))
//     //           .arg(formatBandwidthHz(tr.binHz))
//     //           .arg(res.noisePsdDbmHz, 0, 'f', 1)
//     //           .arg(QStringLiteral("  (wheel: zoom, dbl-click: reset)")));
//     // if (monitor_.valid) {
//     //     QString rng = QString("RANGE %1 - %2")
//     //     .arg(formatFrequencyHz(monitor_.rangeLoHz))
//     //         .arg(formatFrequencyHz(monitor_.rangeHiHz));
//     //     if (monitor_.mode == csm::monitor::Mode::Scan)
//     //         rng += QString("  STEP %1/%2 (only this window is monitored)")
//     //                    .arg(monitor_.stepIndex + 1).arg(monitor_.stepCount);
//     //     line1(rng);
//     // }
//     // int drawn = 0;
//     // for (const csm::spectrum::EmitterNote &e : res.emitters) {
//     //     if (!e.drawn) continue;
//     //     if (++drawn > 2) break;
//     //     line1(QString("SIG %1 BW %2 peak PSD %3 dBm/Hz")
//     //               .arg(formatFrequencyHz(e.frequencyHz))
//     //               .arg(formatBandwidthHz(e.occupiedBandwidthHz))
//     //               .arg(e.peakPsdDbmHz, 0, 'f', 1));
//     // }
//     // if (res.skippedCount > 0) {
//     //     for (const csm::spectrum::EmitterNote &e : res.emitters) {
//     //         if (e.drawn) continue;
//     //         line1(QString("NOT DRAWN: %1: %2%3")
//     //                   .arg(QString::fromStdString(e.label))
//     //                   .arg(QString::fromStdString(e.note))
//     //                   .arg(res.skippedCount > 1 ? QString(" (+%1 more)").arg(res.skippedCount - 1) : QString()));
//     //         break;
//     //     }
//     // }
//     p.restore();
// }

// /* Draw observations table */
// void CSMDisplay::drawObservations(QPainter &p, const QRect &r)
// {
//     visibleRows = 0;
//     if (r.width() < 60 || r.height() < 40) return;

//     p.save();
//     p.setPen(QPen(radarGreen, 1));
//     p.setBrush(Qt::NoBrush);
//     p.drawRect(r);

//     const QFont titleFont = displayFont(1);
//     const QFontMetrics fm(titleFont);
//     p.setFont(titleFont);
//     p.drawText(r.left() + 10, r.top() + 4 + fm.ascent(), "OBSERVATIONS");

//     const int titleH = fm.height() + 8;
//     const QRect tbl(r.left() + 8, r.top() + titleH, r.width() - 16, r.height() - titleH - 8);

//     const QFont tf = displayFont(-1);
//     const QFontMetrics tfm(tf);
//     const int headerH = tfm.height() + 8;
//     const int rowH = tfm.height() + 6;
//     if (tbl.height() < headerH + 4) {
//         p.restore();
//         return;
//     }

//     // Body geometry and scroll clamp
//     const QRect body(tbl.left() + 1, tbl.top() + headerH + 1,
//                      tbl.width() - 1, tbl.height() - headerH - 1);
//     visibleRows = body.height() / rowH;
//     rowScroll = qBound(0, rowScroll, qMax(0, rows.size() - visibleRows));

//     // Title-line note: scroll position and/or data provenance
//     QString note;
//     if (!rows.isEmpty() && visibleRows > 0 && rows.size() > visibleRows)
//         note = QString("%1-%2/%3 ").arg(rowScroll + 1)
//                    .arg(qMin(rows.size(), rowScroll + visibleRows)).arg(rows.size());
//     // if (showsConfiguredValues)
//     //     note += "FREQ/BW/MOD/ID: SIM CONFIG, NOT MEASURED. MOD NOT CLASSIFIED.";
//     // if (showsCalculatedLevel)
//     //     note += QString(note.isEmpty() ? "" : "  ") + "LEVEL/SNR/RNG/BRG: CALCULATED (MODEL)";
//     const int noteAvail = r.width() - 20 - fm.horizontalAdvance("OBSERVATIONS") - 12;
//     if (!note.isEmpty() && noteAvail > 20) {
//         p.setFont(tf);
//         p.drawText(QRect(r.right() - 10 - noteAvail, r.top() + 4, noteAvail, fm.height()),
//                    Qt::AlignRight | Qt::AlignVCenter,
//                    tfm.elidedText(note, Qt::ElideRight, noteAvail));
//     }

//     // Table frame and header
//     p.setFont(tf);
//     p.drawRect(tbl);
//     p.drawLine(tbl.left(), tbl.top() + headerH, tbl.right(), tbl.top() + headerH);

//     int edges[kColumnCount + 1];
//     double cum = 0.0;
//     edges[0] = tbl.left();
//     for (int c = 0; c < kColumnCount; ++c) {
//         cum += kColumnFraction[c];
//         edges[c + 1] = (c == kColumnCount - 1) ? tbl.right()
//                                                : tbl.left() + qRound(tbl.width() * cum);
//     }
//     for (int c = 0; c < kColumnCount; ++c) {
//         if (c > 0)
//             p.drawLine(edges[c], tbl.top() + 3, edges[c], tbl.top() + headerH - 3);
//         const int cw = edges[c + 1] - edges[c] - 4;
//         p.drawText(QRect(edges[c] + 2, tbl.top(), cw, headerH), Qt::AlignCenter,
//                    tfm.elidedText(kColumnTitle[c], Qt::ElideRight, qMax(0, cw)));
//     }

//     // Body
//     p.setClipRect(body);
//     if (rows.isEmpty()) {
//         p.drawText(body, Qt::AlignCenter, observationsMessage());
//     } else {
//         for (int i = 0; i < visibleRows; ++i) {
//             const int idx = rowScroll + i;
//             if (idx >= rows.size()) break;
//             const int y = body.top() + i * rowH;
//             for (int c = 0; c < kColumnCount && c < rows[idx].size(); ++c) {
//                 const int cw = edges[c + 1] - edges[c] - 4;
//                 p.drawText(QRect(edges[c] + 2, y, cw, rowH), Qt::AlignCenter,
//                            tfm.elidedText(rows[idx].at(c), Qt::ElideRight, qMax(0, cw)));
//             }
//         }
//     }
//     p.restore();
// }

// /* Position child widgets (sensor dropdown sits left of the mode indicator) */
// void CSMDisplay::layoutChildren()
// {
//     if (!sensorDropdown) return;
//     QRect spectrum, observations;
//     sectionRects(spectrum, observations);
//     const QFontMetrics fm(displayFont());
//     const int dropW = 110;
//     const int dropH = qMax(18, fm.height() + 4);
//     const int x = spectrum.right() - 10 - fm.horizontalAdvance(modeLabel()) - 8 - dropW;
//     sensorDropdown->setGeometry(qMax(spectrum.left() + 60, x), spectrum.top() + 4, dropW, dropH);
// }

// void CSMDisplay::resizeEvent(QResizeEvent *event)
// {
//     QWidget::resizeEvent(event);
//     layoutChildren();
// }

// void CSMDisplay::updateDropdown()
// {
//     if (!sensorDropdown) return;

//     QSignalBlocker blocker(sensorDropdown);
//     sensorDropdown->clear();

//     if (sensorlist.isEmpty()) {
//         sensorDropdown->hide();
//         return;
//     }

//     for (int i = 0; i < sensorlist.size(); ++i) {
//         Sensor* s = sensorlist[i];
//         QString name = s ? QString::fromStdString(s->Name) : QString("CSM %1").arg(i + 1);
//         if (name.trimmed().isEmpty())
//             name = QString("CSM %1").arg(i + 1);
//         sensorDropdown->addItem(name);
//     }

//     int currentIdx = sensorlist.indexOf(sensor);
//     if (currentIdx >= 0)
//         sensorDropdown->setCurrentIndex(currentIdx);

//     if (sensorlist.size() > 1)
//         sensorDropdown->show();
//     else
//         sensorDropdown->hide();

//     layoutChildren();
// }

// void CSMDisplay::onSensorSelected(int index)
// {
//     if (index < 0 || index >= sensorlist.size()) return;

//     Sensor* s = sensorlist[index];
//     if (!s) return;

//     sensor = s;
//     setRange(sensor->range);
//     rowScroll = 0;
//     scanObserved = false;   // different sensor: no scan seen yet
//     viewZoomed_ = false;
//     refreshObservations(false);
//     update();
// }

















//============================================================================
// FILE:         csmdisplay.cpp
// MODULE:       CSM (Communications Support Measures) Display
// PROJECT:      Indigenous Scenario and Sensor Simulation Toolkit (ISSST)
// ORGANISATION: Oxygen 2 Innovation (O2I).
// STANDARD:     RTCA DO-178C / ED-12C, DAL B
// COVERAGE:     Branch / Decision Coverage required (100% true/false paths)
//
// DESCRIPTION:  Implements the CSMDisplay class: a frequency-spectrum frame
//               (modelled PSD trace from CSM::lastSpectrum()) above
//               an observations table fed from the selected CSM sensor's
//               per-radio scan result (CSM::lastScanResult()). See
//               csmdisplay.h for the data policy.
//
// REQUIREMENTS: Implements REQ-CSM-010, REQ-CSM-015, REQ-CSM-016, REQ-CSM-017
//
// AUTHOR:       Arti Rajpoot
// REVIEWED BY:  [Reviewer Name], [Review Date] — SPR-CSM-001
//
//
// COPYRIGHT:    Oxygen 2 Innovation (O2I). All rights reserved.
//               Restricted circulation — defence simulation use only.
//============================================================================

#include "csmdisplay.h"                            // For CSM display class
#include "core/Hierarchy/Utils/entityutils.h"
#include "core/Hierarchy/EntityProfiles/SensorProfiles/csm.h"   // Per-radio scan result
#include "qelapsedtimer.h"
#include <QPainter>                                // For painting operations
#include <QPaintEvent>                             // For paint events
#include <QMouseEvent>                             // For double-click
#include <QPolygonF>                              // For spectrum trace
#include <QWheelEvent>                             // For table scrolling
#include <QFont>                                   // For font settings
#include <QFontMetrics>                            // For text layout
#include <QtMath>                                  // For math functions
#include <QDebug>                                  // For debug output
#include <cmath>
#include <core/Debug/console.h>                    // For console error logging

namespace {

const int kColumnCount = 8;
// Column width fractions (sum = 1.0)
const double kColumnFraction[kColumnCount] = { 0.05, 0.15, 0.12, 0.14, 0.12, 0.09, 0.15, 0.18 };
const char* const kColumnTitle[kColumnCount] = {
    "#", "FREQUENCY", "BANDWIDTH", "MOD (CFG)", "LEVEL", "SNR", "RNG / BRG", "EMITTER"
};
// const char* const kColumnTitle[kColumnCount] = {
//     "#", "FREQUENCY", "BANDWIDTH", "MOD (CFG)", "LEVEL", "EMITTER"
// };
const char* const kPlaceholder = "--";

QString formatFrequencyHz(double hz)
{
    if (!(hz > 0.0) || !std::isfinite(hz)) return kPlaceholder;
    if (hz >= 1e9) return QString::number(hz / 1e9, 'f', 3) + " GHz";
    if (hz >= 1e6) return QString::number(hz / 1e6, 'f', 3) + " MHz";
    return QString::number(hz / 1e3, 'f', 1) + " kHz";
}

QString formatBandwidthHz(double hz)
{
    if (!(hz > 0.0) || !std::isfinite(hz)) return kPlaceholder;
    if (hz >= 1e6) return QString::number(hz / 1e6, 'f', 2) + " MHz";
    if (hz >= 1e3) return QString::number(hz / 1e3, 'f', 1) + " kHz";
    return QString::number(hz, 'f', 0) + " Hz";
}

QString formatSnr(const csm::Quantity& q)
{
    // Calculated SNR over the signal's occupied bandwidth (model result).
    if (!q.valid()) return kPlaceholder;
    return QString::number(q.value, 'f', 1) + " dB";
}

QString formatRangeBearing(const csm::Quantity& range, const csm::Quantity& bearing)
{
    // Calculated geometry: great-circle range [km] and bearing in the
    // observer's local frame [deg, 0-360].
    if (!range.valid() || !bearing.valid()) return kPlaceholder;
    double b = std::fmod(bearing.value, 360.0);
    if (b < 0.0) b += 360.0;
    return QString("%1 km / %2%3")
        .arg(range.value / 1000.0, 0, 'f', 1)
        .arg(b, 0, 'f', 0)
        .arg(QChar(0x00B0));
}


QString formatLevel(const csm::Quantity& q)
{
    // Calculated link-budget level (model result, not a measurement).
    if (!q.valid()) return kPlaceholder;
    return QString::number(q.value, 'f', 1) + " dBm";
}

} // namespace

// %%% Constructor %%%
/* Initialize CSM display */
CSMDisplay::CSMDisplay(QWidget *parent)
    : QWidget(parent)
{
    // Set background color
    setStyleSheet("background-color: black;");
    // Set size policy with aspect ratio
    QSizePolicy policy(QSizePolicy::Preferred, QSizePolicy::Preferred);
    policy.setHeightForWidth(true);
    setSizePolicy(policy);
    padding = 6;
    sensorDropdown = new QComboBox(this);
    sensorDropdown->setStyleSheet(
        "QComboBox { background-color: #001a00; color: #00ff00; "
        "border: 1px solid #00ff00; font-size: 10px; padding: 2px; }"
        "QComboBox QAbstractItemView { background-color: #001a00; "
        "color: #00ff00; selection-background-color: #003300; }"
        );
    sensorDropdown->hide();
    connect(sensorDropdown, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &CSMDisplay::onSensorSelected);
}

// %%% Size Management %%%
/* Provide size hint for widget */
QSize CSMDisplay::sizeHint() const
{
    int defaultWidth = 400;
    return QSize(defaultWidth, heightForWidth(defaultWidth));
}

/* Provide minimum size for widget */
QSize CSMDisplay::minimumSize() const
{
    int minW = 250;
    return QSize(minW, heightForWidth(minW));
}

/* Calculate height based on width and aspect ratio */
int CSMDisplay::heightForWidth(int width) const
{
    return qRound(width * ASPECT_RATIO);
}

/* Mouse move: no hover behaviour in the spectrum layout */
void CSMDisplay::mouseMoveEvent(QMouseEvent *event)
{
    QWidget::mouseMoveEvent(event);
}

/* Double-click: reset wheel zoom or toggle strongest-signal zoom */
void CSMDisplay::mouseDoubleClickEvent(QMouseEvent *event)
{
    if (viewZoomed_) {
        viewZoomed_ = false;
        update();
        event->accept();
        return;
    }

    if (spectrum_ && spectrum_->available && spectrum_->hasZoom) {
        zoomView_ = !zoomView_;
        update();
        event->accept();
        return;
    }

    QWidget::mouseDoubleClickEvent(event);
}

/* Handle mouse leave events */
void CSMDisplay::leaveEvent(QEvent *event)
{
    QWidget::leaveEvent(event);
}

/* Scroll the observations table */
/* Spectrum: mouse-wheel zoom. Observations table: normal row scrolling. */
void CSMDisplay::wheelEvent(QWheelEvent *event)
{
    const double notches = event->angleDelta().y() / 120.0;

    // Qt 5 compatible mouse position.
    const QPoint pos = event->pos();

    // If the mouse is over the spectrum plot, zoom its frequency range.
    if (notches != 0.0 &&
        spectrum_ &&
        spectrum_->available &&
        spectrumPlot_.isValid() &&
        spectrumPlot_.contains(pos)) {

        if (applyWheelZoom(qBound(-10.0, notches, 10.0), pos.x())) {
            update();
        }

        event->accept();
        return;
    }

    // Otherwise retain the existing observations-table scrolling.
    const int maxScroll = qMax(0, rows.size() - visibleRows);
    const int steps = event->angleDelta().y() / 120;

    if (maxScroll == 0 || steps == 0) {
        event->ignore();
        return;
    }

    const int next = qBound(0, rowScroll - steps, maxScroll);

    if (next != rowScroll) {
        rowScroll = next;
        update();
    }

    event->accept();
}


/* Apply wheel zoom while keeping the frequency under the cursor fixed. */
// bool CSMDisplay::applyWheelZoom(double notches, int xPixel)
// {
//     if (!spectrum_ ||
//         !spectrum_->available ||
//         spectrumPlot_.width() < 2) {
//         return false;
//     }

//     const double fullLo = spectrum_->span.startHz;
//     const double fullHi = spectrum_->span.stopHz();
//     const double fullW = fullHi - fullLo;

//     if (!std::isfinite(fullW) || !(fullW > 0.0)) {
//         return false;
//     }

//     double lo = fullLo;
//     double hi = fullHi;

//     if (viewZoomed_) {
//         lo = qMax(fullLo, viewLoHz_);
//         hi = qMin(fullHi, viewHiHz_);

//         if (!(hi > lo)) {
//             lo = fullLo;
//             hi = fullHi;
//         }
//     }

//     const double w = hi - lo;

//     const double t = qBound(
//         0.0,
//         double(xPixel - spectrumPlot_.left()) / spectrumPlot_.width(),
//         1.0
//         );

//     // Frequency beneath the cursor remains the zoom anchor.
//     const double anchor = lo + t * w;

//     const double newW = qBound(
//         qMin(kMinViewHz, fullW),
//         w * std::pow(kWheelStep, notches),
//         fullW
//         );

//     // Reaching the full range resets wheel zoom.
//     if (newW >= fullW * (1.0 - 1e-9)) {
//         const bool wasZoomed = viewZoomed_;
//         viewZoomed_ = false;
//         return wasZoomed;
//     }

//     const double newLo = qBound(
//         fullLo,
//         anchor - t * newW,
//         fullHi - newW
//         );

//     viewLoHz_ = newLo;
//     viewHiHz_ = newLo + newW;
//     viewZoomed_ = true;

//     return true;
// }
/* Apply wheel zoom while keeping the frequency under the cursor fixed. */
bool CSMDisplay::applyWheelZoom(double notches, int xPixel)
{
    if (!spectrum_ ||
        !spectrum_->available ||
        spectrumPlot_.width() < 2) {
        return false;
    }

    const double fullLo = spectrum_->span.startHz;
    const double fullHi = spectrum_->span.stopHz();
    const double fullW = fullHi - fullLo;

    if (!std::isfinite(fullW) || !(fullW > 0.0)) {
        return false;
    }

    double lo = fullLo;
    double hi = fullHi;

    if (viewZoomed_) {
        lo = qMax(fullLo, viewLoHz_);
        hi = qMin(fullHi, viewHiHz_);

        if (!(hi > lo)) {
            lo = fullLo;
            hi = fullHi;
        }
    }

    const double w = hi - lo;

    const double t = qBound(
        0.0,
        double(xPixel - spectrumPlot_.left()) / spectrumPlot_.width(),
        1.0
        );

    // Frequency beneath the cursor remains the zoom anchor.
    const double anchor = lo + t * w;

    const double newW = qBound(
        qMin(kMinViewHz, fullW),
        w * std::pow(kWheelStep, notches),
        fullW
        );

    // Reaching the full range resets wheel zoom.
    if (newW >= fullW * (1.0 - 1e-9)) {
        const bool wasZoomed = viewZoomed_;
        viewZoomed_ = false;
        return wasZoomed;
    }

    const double newLo = qBound(
        fullLo,
        anchor - t * newW,
        fullHi - newW
        );

    viewLoHz_ = newLo;
    viewHiHz_ = newLo + newW;
    viewZoomed_ = true;

    return true;
}


/* Return the valid wheel-zoom window, clipped to the monitor span. */
bool CSMDisplay::effectiveView(
    const csm::spectrum::Result &res,
    double &lo,
    double &hi
    ) const
{
    if (!viewZoomed_) {
        return false;
    }

    const double fullW = res.span.stopHz() - res.span.startHz;

    lo = qMax(res.span.startHz, viewLoHz_);
    hi = qMin(res.span.stopHz(), viewHiHz_);

    return std::isfinite(lo) &&
           std::isfinite(hi) &&
           hi > lo &&
           (hi - lo) >= qMin(kMinViewHz, fullW) * 0.999;
}



/* Select entity for display */
void CSMDisplay::selectEntity(Entity* entit)
{
    entity = nullptr;
    id = "";
    rowScroll = 0;
    zoomView_ = false;
    viewZoomed_ = false;
    scanObserved = false;   // new selection: no scan seen yet

    Platform* platform = dynamic_cast<Platform*>(entit);
    if (!platform) {
        Console::error("Entity is not a Platform");
        setWindowTitle("CSM Display (No Platform)");
        refreshObservations(false);
        update();
        return;
    }

    // Set entity ID and pointer
    id = QString::fromStdString(platform->ID);
    entity = platform;

    sensor = nullptr;
    sensorlist.clear();
    for (auto const& pair :  entity->sensors.sensors) {
        Sensor* s = pair.second;
        if (s && s->subType == Sensor::SubType::CSM) {
            if(sensor == nullptr){
                sensor = s;
            }
            sensorlist.append(s);
            setWindowTitle("CSM Display (" + QString::fromStdString(entity->Name) + ")");
        }
    }

    updateDropdown();
    refreshObservations(false);
    update();
}

/* Remove entity from display */
void CSMDisplay::RemoveEntity(QString ID)
{
    if (id == ID) {
        entity = nullptr;
        sensor = nullptr;

        sensorlist.clear();
        if (sensorDropdown) {
            QSignalBlocker blocker(sensorDropdown);
            sensorDropdown->clear();
            sensorDropdown->hide();
        }

        id.clear();
        setWindowTitle("CSM Display");
        rowScroll = 0;
        refreshObservations(false);

        update();
    }
}

/* Update display with new data (called on every simulation update) */
void CSMDisplay::updateRadar()
{
    if (entity && sensor) {
        bool stillValid = false;
        for (auto const& pair : entity->sensors.sensors) {
            if (pair.second == sensor) { stillValid = true; break; }
        }
        if (!stillValid) sensor = nullptr;
    }

    if (entity && sensor) {
        setRange(sensor->range);
        if (refreshObservations(true)) update();
    } else {
        if (refreshObservations(false)) update();
    }
}

// %%% Data Methods %%%
/* Rebuild cached rows and state from the selected sensor.
 * scanTick is true only when called from the periodic update, so that
 * selecting a sensor does not by itself count as a completed scan. */
bool CSMDisplay::refreshObservations(bool scanTick)
{
    const bool newActive = (entity && sensor && sensor->Active);

    bool newScan = scanObserved;
    if (!newActive)      newScan = false;
    else if (scanTick)   newScan = true;

    QVector<QStringList> newRows;
    bool configured = false;
    bool calculated = false;
    const CSM* csmSensor = newActive ? dynamic_cast<const CSM*>(sensor) : nullptr;
    // Cached modelled spectrum (shared pointer: no copy, changes only on recompute).
    const std::shared_ptr<const csm::spectrum::Result> newSpectrum =
        csmSensor ? csmSensor->lastSpectrum() : nullptr;
    const csm::monitor::Window newMonitor =
        csmSensor ? csmSensor->lastMonitor() : csm::monitor::Window();
    if (csmSensor) {
        int n = 0;
        for (const csm::Observation &o : csmSensor->lastScanResult().observations) {
            ++n;
            QString freq = kPlaceholder, bw = kPlaceholder, mod = kPlaceholder;
            if (o.frequencyHz.valid()) { freq = formatFrequencyHz(o.frequencyHz.value); configured = true; }
            if (o.bandwidthHz.valid()) { bw = formatBandwidthHz(o.bandwidthHz.value); configured = true; }
            if (o.modulationSource == csm::Provenance::Configured && !o.modulation.empty()) {
                mod = QString::fromStdString(o.modulation);
                configured = true;
            }
            if (o.levelDbm.valid() || o.snrDb.valid() ||
                o.distanceM.valid() || o.bearingDeg.valid()) calculated = true;


            const QString platformName = QString::fromStdString(
                o.platformName.empty() ? o.platformId : o.platformName);
            const QString radioName = QString::fromStdString(
                o.radioName.empty() ? o.radioId : o.radioName);

            QStringList row;
            row << QString::number(n) << freq << bw << mod
                << formatLevel(o.levelDbm)
                << formatSnr(o.snrDb)
                << formatRangeBearing(o.distanceM, o.bearingDeg)
                << (platformName + "/" + radioName);
            newRows.append(row);
        }
    }

    const bool changed = (newActive != sensorActive) || (newScan != scanObserved)
                         || (configured != showsConfiguredValues)
                         || (calculated != showsCalculatedLevel) || (newRows != rows)
                         || (newSpectrum != spectrum_)
                         || (newMonitor.valid != monitor_.valid) || (newMonitor.mode != monitor_.mode)
                         || (newMonitor.autoRange != monitor_.autoRange)
                         || (newMonitor.stepIndex != monitor_.stepIndex)
                         || (newMonitor.centreHz != monitor_.centreHz)
                         || (newMonitor.spanHz != monitor_.spanHz);


    // Reset wheel zoom when the monitored frequency window changes.
    if (viewZoomed_ &&
        (!newSpectrum ||
         !newSpectrum->available ||
         newMonitor.centreHz != monitor_.centreHz ||
         newMonitor.spanHz != monitor_.spanHz)) {
        viewZoomed_ = false;
    }
    sensorActive = newActive;
    scanObserved = newScan;
    showsConfiguredValues = configured;
    showsCalculatedLevel = calculated;
    rows = newRows;
    spectrum_ = newSpectrum;
    monitor_ = newMonitor;
    return changed;
}

/* Text shown in the empty observations table */
QString CSMDisplay::observationsMessage() const
{
    if (!entity || !sensor) return "No CSM Sensor";
    if (!sensorActive)      return "CSM Sensor Inactive";
    if (const CSM* c = dynamic_cast<const CSM*>(sensor))
        if (c->operatingMode() == csm::OperatingMode::Transmit)
            return c->txEnabled ? "CSM transmitting (receiver suspended)"
                                : "CSM in TRANSMIT mode, transmission off";
    if (!scanObserved)      return "Waiting for First Scan";
    return "No CSM Signals Detected";
}

/* Mode text from the actual configuration: MONITOR/SCAN and AUTO/MANUAL range */
QString CSMDisplay::modeLabel() const
{
    const CSM* csmSensor = sensorActive ? dynamic_cast<const CSM*>(sensor) : nullptr;
    if (csmSensor && csmSensor->operatingMode() == csm::OperatingMode::Transmit)
        return QString("MODE : TRANSMIT (%1)").arg(csmSensor->txEnabled ? "ON" : "OFF");
    // Receiver: show only that interference is present (no source identity).
    if (csmSensor && csmSensor->lastScanResult().interference.present)
        return QString("MODE : %1 (%2)  INTERFERENCE DETECTED")
            .arg(QString::fromLatin1(csm::monitor::modeName(monitor_.mode)))
            .arg(monitor_.autoRange ? QStringLiteral("AUTO") : QStringLiteral("MANUAL"));
    return QString("MODE : %1 (%2)")
        .arg(QString::fromLatin1(csm::monitor::modeName(monitor_.mode)))
        .arg(monitor_.autoRange ? QStringLiteral("AUTO") : QStringLiteral("MANUAL"));
}

/* Monospace display font scaled with widget width */
QFont CSMDisplay::displayFont(int delta, bool bold) const
{
    QFont f("Monospace");
    f.setStyleHint(QFont::TypeWriter);
    f.setPixelSize(qMax(7, qBound(9, width() / 46, 15) + delta));
    f.setBold(bold);
    return f;
}

/* Split the widget into spectrum (top) and observations (bottom) */
void CSMDisplay::sectionRects(QRect &spectrum, QRect &observations) const
{
    const QRect total = rect().adjusted(padding, padding, -padding, -padding);
    const int specH = qRound(total.height() * 0.70);
    spectrum = QRect(total.left(), total.top(), total.width(), specH);
    observations = QRect(total.left(), total.top() + specH + padding,
                         total.width(), total.height() - specH - padding);
}

/* Main paint event handler */
void CSMDisplay::paintEvent(QPaintEvent * /*event*/)
{
    QElapsedTimer timer;
    timer.start();

    if (width() <= 0 || height() <= 0) return;

    QPainter p(this);
    p.fillRect(rect(), Qt::black);

    QRect spectrum, observations;
    sectionRects(spectrum, observations);
    drawSpectrum(p, spectrum);
    drawObservations(p, observations);

    if (Profiler::currentFrame)
        Profiler::currentFrame->csmdisplay = timer.elapsed();
}
void CSMDisplay::drawSpectrum(QPainter &p, const QRect &r)
{

    spectrumPlot_ = QRect();
    if (r.width() < 40 || r.height() < 40) return;

    p.save();
    p.setPen(QPen(radarGreen, 1));
    p.setBrush(Qt::NoBrush);
    p.drawRect(r);

    // Header: title (left), monitor-mode indicator (right)
    const QFont titleFont = displayFont(4, true);
    const QFontMetrics tfm(titleFont);
    p.setFont(titleFont);
    p.drawText(r.left() + 10, r.top() + 4 + tfm.ascent(), "CSM");

    const QFont f = displayFont();
    const QFontMetrics fm(f);
    p.setFont(f);
    const QString mode = modeLabel();
    p.drawText(r.right() - 10 - fm.horizontalAdvance(mode),
               r.top() + 4 + (tfm.height() - fm.height()) / 2 + fm.ascent(), mode);

    // Plot area
    const int headerH = tfm.height() + 8;
    const QRect plot(QPoint(r.left() + fm.height() * 2 + 6, r.top() + headerH + 4),
                     QPoint(r.right() - 12, r.bottom() - fm.height() - 18));
    if (plot.width() < 30 || plot.height() < 30) {
        p.restore();
        return;
    }
    spectrumPlot_ = plot;

    // Axes
    p.setPen(QPen(radarGreen, 1));
    p.drawLine(plot.left(), plot.top(), plot.left(), plot.bottom());
    p.drawLine(plot.left(), plot.bottom(), plot.right(), plot.bottom());

    // Y ticks: major (long) at grid lines, minor (short) between them
    const double step = plot.height() / 5.8;
    for (int n = 0; n <= 11; ++n) {
        const int y = qRound(plot.top() + n * 0.5 * step);
        const int len = (n % 2 == 1) ? 9 : 5;
        p.drawLine(plot.left() - len, y, plot.left(), y);
    }
    // X ticks
    for (int i = 0; i <= 6; ++i) {
        const int x = qRound(plot.left() + plot.width() * i / 6.0);
        p.drawLine(x, plot.bottom(), x, plot.bottom() + 8);
    }

    // Horizontal dashed grid lines
    QPen dash(radarGreen, 1);
    dash.setStyle(Qt::CustomDashLine);
    dash.setDashPattern(QVector<qreal>() << 3 << 3);
    p.setPen(dash);
    for (int k = 0; k < 6; ++k) {
        const int y = qRound(plot.top() + (0.5 + k) * step);
        p.drawLine(plot.left() + 1, y, plot.right() - 6, y);
    }

    const bool haveSpectrum = spectrum_ && spectrum_->available;

    // Axis titles
    p.setPen(QPen(radarGreen, 1));
    p.save();
    p.translate(r.left() + fm.height(), plot.center().y());
    p.rotate(-90);
    p.drawText(QRect(-plot.height() / 2, -fm.height() / 2, plot.height(), fm.height()),
               Qt::AlignCenter, haveSpectrum ? "PSD  dBm/Hz" : "LEVEL");
    p.restore();

    if (haveSpectrum) {
        drawSpectrumTrace(p, plot);
        p.restore();
        return;
    }

    p.drawText(QRect(plot.left(), plot.bottom() + 10, plot.width(), fm.height() + 4),
               Qt::AlignCenter, "FREQUENCY");

    // No valid spectrum: say why (never draw a placeholder trace)
    QString msg;
    if (spectrum_)               msg = "Spectrum unavailable: " + QString::fromStdString(spectrum_->reason);
    else if (!entity || !sensor) msg = "Spectrum unavailable: no CSM sensor";
    else if (!sensorActive)      msg = "Spectrum unavailable: CSM sensor inactive";
    else if (const CSM* c = dynamic_cast<const CSM*>(sensor);
             c && c->operatingMode() == csm::OperatingMode::Transmit)
        msg = "Spectrum unavailable: CSM is transmitting (receiver suspended)";
    else                         msg = "Spectrum unavailable: waiting for first scan";
    p.setFont(displayFont(1));
    p.drawText(plot.adjusted(10, 10, -10, -10), Qt::AlignCenter | Qt::TextWordWrap, msg);
    p.restore();
}

/* Draw the modelled PSD trace. All numbers come from the cached
 * csm::spectrum::Result; nothing is calculated here except pixel positions. */
void CSMDisplay::drawSpectrumTrace(QPainter &p, const QRect &plot)
{
    // const csm::spectrum::Result &res = *spectrum_;
    // const bool zoom = zoomView_ && res.hasZoom;
    // const csm::spectrum::Trace &tr = zoom ? res.zoom : res.span;
    // const int n = static_cast<int>(tr.psdDbmHz.size());
    // double lo = 0.0, hi = 0.0;
    // if (n < 2 || !(tr.binHz > 0.0) || !csm::spectrum::axisRange(tr, lo, hi)) return;


    const csm::spectrum::Result &res = *spectrum_;

    // Mouse-wheel zoom is evaluated using the same analytical model.
    csm::spectrum::Trace wheelTrace;
    double vLo = 0.0;
    double vHi = 0.0;

    const bool wheel =
        effectiveView(res, vLo, vHi) &&
        csm::spectrum::windowTrace(
            res,
            vLo,
            vHi,
            csm::spectrum::kDisplayBins,
            wheelTrace
            );

    // Priority: wheel zoom > existing double-click zoom > full spectrum.
    const bool zoom = !wheel && zoomView_ && res.hasZoom;

    const csm::spectrum::Trace &tr =
        wheel ? wheelTrace : (zoom ? res.zoom : res.span);

    const int n = static_cast<int>(tr.psdDbmHz.size());

    double lo = 0.0;
    double hi = 0.0;

    if (n < 2 ||
        !(tr.binHz > 0.0) ||
        !csm::spectrum::axisRange(tr, lo, hi)) {
        return;
    }

    const QFont small = displayFont(-2);
    const QFontMetrics sfm(small);
    p.save();
    p.setFont(small);
    p.setPen(QPen(radarGreen, 1));

    // Pixel mapping. The six dashed grid lines (drawn by the caller) sit at
    // plot.top() + (0.5 + k) * step; grid line 0 = hi dB, grid line 5 = lo dB.
    const double step = plot.height() / 5.8;
    const double fStart = tr.startHz, fStop = tr.stopHz();
    auto xOf = [&](double f) { return plot.left() + (f - fStart) / (fStop - fStart) * plot.width(); };
    auto yOf = [&](double v) { return plot.top() + 0.5 * step + (hi - v) / (hi - lo) * 5.0 * step; };

    // Y labels on the grid lines (dBm/Hz)
    for (int k = 0; k < 6; ++k) {
        const double v = hi - k * (hi - lo) / 5.0;
        p.drawText(QPoint(plot.left() + 4, qRound(plot.top() + (0.5 + k) * step) - 2),
                   QString::number(v, 'f', 0));
    }

    // X labels: start / centre / stop of the displayed frequency range
    const int ly = plot.bottom() + 10;
    const QRect lr(plot.left(), ly, plot.width(), sfm.height() + 4);
    p.drawText(lr, Qt::AlignLeft | Qt::AlignVCenter, formatFrequencyHz(fStart));
    p.drawText(lr, Qt::AlignHCenter | Qt::AlignVCenter, formatFrequencyHz(0.5 * (fStart + fStop)));
    p.drawText(lr, Qt::AlignRight | Qt::AlignVCenter, formatFrequencyHz(fStop));

    // Trace (green on black), clipped to the plot
    p.setClipRect(plot);
    QPolygonF line;
    line.reserve(n);
    for (int i = 0; i < n; ++i)
        line << QPointF(xOf(tr.binCentreHz(i)), yOf(tr.psdDbmHz[static_cast<size_t>(i)]));
    p.setPen(QPen(radarGreen, 1));
    p.drawPolyline(line);

    // Markers: small triangle on top of the DISPLAYED value at each drawn
    // emitter's configured frequency. Position only; the true peak PSD is
    // quoted in the text below, the trace is not altered.
    for (const csm::spectrum::EmitterNote &e : res.emitters) {
        if (!e.drawn || e.frequencyHz < fStart || e.frequencyHz >= fStop) continue;
        const int bi = qBound(0, static_cast<int>((e.frequencyHz - fStart) / tr.binHz), n - 1);
        const double x = xOf(e.frequencyHz);
        const double y = yOf(tr.psdDbmHz[static_cast<size_t>(bi)]);
        QPolygonF tri;
        tri << QPointF(x - 4, y - 10) << QPointF(x + 4, y - 10) << QPointF(x, y - 3);
        p.drawPolygon(tri);
    }
    p.setClipping(false);

    // // Provenance and scale notes (top of plot)
    // int ty = plot.top() + sfm.ascent() + 2;
    // const int tx = plot.left() + 4;
    // const int tw = plot.width() - 8;
    // auto line1 = [&](const QString &t) {
    //     p.drawText(QPoint(tx, ty), sfm.elidedText(t, Qt::ElideRight, tw));
    //     ty += sfm.height();
    // };
    // line1(QString::fromLatin1(csm::spectrum::provenanceLabel()));
    // line1(QString("%1  bin %2  noise %3 dBm/Hz (thermal model)%4")
    //           .arg(wheel ? QStringLiteral("WHEEL-ZOOM")
    //                      : (zoom ? QStringLiteral("ZOOM") : QStringLiteral("SPAN")))
    //           .arg(formatBandwidthHz(tr.binHz))
    //           .arg(res.noisePsdDbmHz, 0, 'f', 1)
    //           .arg(QStringLiteral("  (wheel: zoom, dbl-click: reset)")));
    // if (monitor_.valid) {
    //     QString rng = QString("RANGE %1 - %2")
    //     .arg(formatFrequencyHz(monitor_.rangeLoHz))
    //         .arg(formatFrequencyHz(monitor_.rangeHiHz));
    //     if (monitor_.mode == csm::monitor::Mode::Scan)
    //         rng += QString("  STEP %1/%2 (only this window is monitored)")
    //                    .arg(monitor_.stepIndex + 1).arg(monitor_.stepCount);
    //     line1(rng);
    // }
    // int drawn = 0;
    // for (const csm::spectrum::EmitterNote &e : res.emitters) {
    //     if (!e.drawn) continue;
    //     if (++drawn > 2) break;
    //     line1(QString("SIG %1 BW %2 peak PSD %3 dBm/Hz")
    //               .arg(formatFrequencyHz(e.frequencyHz))
    //               .arg(formatBandwidthHz(e.occupiedBandwidthHz))
    //               .arg(e.peakPsdDbmHz, 0, 'f', 1));
    // }
    // if (res.skippedCount > 0) {
    //     for (const csm::spectrum::EmitterNote &e : res.emitters) {
    //         if (e.drawn) continue;
    //         line1(QString("NOT DRAWN: %1: %2%3")
    //                   .arg(QString::fromStdString(e.label))
    //                   .arg(QString::fromStdString(e.note))
    //                   .arg(res.skippedCount > 1 ? QString(" (+%1 more)").arg(res.skippedCount - 1) : QString()));
    //         break;
    //     }
    // }
    p.restore();
}

/* Draw observations table */
void CSMDisplay::drawObservations(QPainter &p, const QRect &r)
{
    visibleRows = 0;
    if (r.width() < 60 || r.height() < 40) return;

    p.save();
    p.setPen(QPen(radarGreen, 1));
    p.setBrush(Qt::NoBrush);
    p.drawRect(r);

    const QFont titleFont = displayFont(1);
    const QFontMetrics fm(titleFont);
    p.setFont(titleFont);
    p.drawText(r.left() + 10, r.top() + 4 + fm.ascent(), "OBSERVATIONS");

    const int titleH = fm.height() + 8;
    const QRect tbl(r.left() + 8, r.top() + titleH, r.width() - 16, r.height() - titleH - 8);

    const QFont tf = displayFont(-1);
    const QFontMetrics tfm(tf);
    const int headerH = tfm.height() + 8;
    const int rowH = tfm.height() + 6;
    if (tbl.height() < headerH + 4) {
        p.restore();
        return;
    }

    // Body geometry and scroll clamp
    const QRect body(tbl.left() + 1, tbl.top() + headerH + 1,
                     tbl.width() - 1, tbl.height() - headerH - 1);
    visibleRows = body.height() / rowH;
    rowScroll = qBound(0, rowScroll, qMax(0, rows.size() - visibleRows));

    // Title-line note: scroll position and/or data provenance
    QString note;
    if (!rows.isEmpty() && visibleRows > 0 && rows.size() > visibleRows)
        note = QString("%1-%2/%3 ").arg(rowScroll + 1)
                   .arg(qMin(rows.size(), rowScroll + visibleRows)).arg(rows.size());
    // if (showsConfiguredValues)
    //     note += "FREQ/BW/MOD/ID: SIM CONFIG, NOT MEASURED. MOD NOT CLASSIFIED.";
    // if (showsCalculatedLevel)
    //     note += QString(note.isEmpty() ? "" : "  ") + "LEVEL/SNR/RNG/BRG: CALCULATED (MODEL)";
    const int noteAvail = r.width() - 20 - fm.horizontalAdvance("OBSERVATIONS") - 12;
    if (!note.isEmpty() && noteAvail > 20) {
        p.setFont(tf);
        p.drawText(QRect(r.right() - 10 - noteAvail, r.top() + 4, noteAvail, fm.height()),
                   Qt::AlignRight | Qt::AlignVCenter,
                   tfm.elidedText(note, Qt::ElideRight, noteAvail));
    }

    // Table frame and header
    p.setFont(tf);
    p.drawRect(tbl);
    p.drawLine(tbl.left(), tbl.top() + headerH, tbl.right(), tbl.top() + headerH);

    int edges[kColumnCount + 1];
    double cum = 0.0;
    edges[0] = tbl.left();
    for (int c = 0; c < kColumnCount; ++c) {
        cum += kColumnFraction[c];
        edges[c + 1] = (c == kColumnCount - 1) ? tbl.right()
                                               : tbl.left() + qRound(tbl.width() * cum);
    }
    for (int c = 0; c < kColumnCount; ++c) {
        if (c > 0)
            p.drawLine(edges[c], tbl.top() + 3, edges[c], tbl.top() + headerH - 3);
        const int cw = edges[c + 1] - edges[c] - 4;
        p.drawText(QRect(edges[c] + 2, tbl.top(), cw, headerH), Qt::AlignCenter,
                   tfm.elidedText(kColumnTitle[c], Qt::ElideRight, qMax(0, cw)));
    }

    // Body
    p.setClipRect(body);
    if (rows.isEmpty()) {
        p.drawText(body, Qt::AlignCenter, observationsMessage());
    } else {
        for (int i = 0; i < visibleRows; ++i) {
            const int idx = rowScroll + i;
            if (idx >= rows.size()) break;
            const int y = body.top() + i * rowH;
            for (int c = 0; c < kColumnCount && c < rows[idx].size(); ++c) {
                const int cw = edges[c + 1] - edges[c] - 4;
                p.drawText(QRect(edges[c] + 2, y, cw, rowH), Qt::AlignCenter,
                           tfm.elidedText(rows[idx].at(c), Qt::ElideRight, qMax(0, cw)));
            }
        }
    }
    p.restore();
}

/* Position child widgets (sensor dropdown sits left of the mode indicator) */
void CSMDisplay::layoutChildren()
{
    if (!sensorDropdown) return;
    QRect spectrum, observations;
    sectionRects(spectrum, observations);
    const QFontMetrics fm(displayFont());
    const int dropW = 110;
    const int dropH = qMax(18, fm.height() + 4);
    const int x = spectrum.right() - 10 - fm.horizontalAdvance(modeLabel()) - 8 - dropW;
    sensorDropdown->setGeometry(qMax(spectrum.left() + 60, x), spectrum.top() + 4, dropW, dropH);
}

void CSMDisplay::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    layoutChildren();
}

void CSMDisplay::updateDropdown()
{
    if (!sensorDropdown) return;

    QSignalBlocker blocker(sensorDropdown);
    sensorDropdown->clear();

    if (sensorlist.isEmpty()) {
        sensorDropdown->hide();
        return;
    }

    for (int i = 0; i < sensorlist.size(); ++i) {
        Sensor* s = sensorlist[i];
        QString name = s ? QString::fromStdString(s->Name) : QString("CSM %1").arg(i + 1);
        if (name.trimmed().isEmpty())
            name = QString("CSM %1").arg(i + 1);
        sensorDropdown->addItem(name);
    }

    int currentIdx = sensorlist.indexOf(sensor);
    if (currentIdx >= 0)
        sensorDropdown->setCurrentIndex(currentIdx);

    if (sensorlist.size() > 1)
        sensorDropdown->show();
    else
        sensorDropdown->hide();

    layoutChildren();
}

void CSMDisplay::onSensorSelected(int index)
{
    if (index < 0 || index >= sensorlist.size()) return;

    Sensor* s = sensorlist[index];
    if (!s) return;

    sensor = s;
    setRange(sensor->range);
    rowScroll = 0;
    scanObserved = false;   // different sensor: no scan seen yet
    viewZoomed_ = false;
    refreshObservations(false);
    update();
}

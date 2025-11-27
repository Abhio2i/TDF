#ifndef SCALEBAR_H
#define SCALEBAR_H

#include <QWidget>

class ScaleBar : public QWidget
{
    Q_OBJECT

public:
    explicit ScaleBar(QWidget *parent = nullptr);

    // Update scale based on actual map parameters
    void updateScale(double metersPerPixel, int containerWidth);
    QSize sizeHint() const override;

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    struct ScaleConfig {
        int barLength;
        QString displayText;
        double actualDistance; // meters
    };

    ScaleConfig getOptimalScale(double metersPerPixel, int containerWidth) const;
    bool isNiceNumber(double num) const;
    QString formatDistance(double distance) const; // REMOVED unit parameter

    ScaleConfig m_currentConfig;
};

#endif // SCALEBAR_H

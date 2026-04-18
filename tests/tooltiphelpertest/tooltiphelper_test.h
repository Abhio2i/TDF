#ifndef TOOLTIPHELPER_TEST_H
#define TOOLTIPHELPER_TEST_H

#include <QObject>

class TestTooltipHelper : public QObject
{
    Q_OBJECT

private slots:
    void testGetAvailableFields();
    void testFormatTooltipHTML();
    void testFormatCompactTooltip();
    void testGetDynamicModelInfoWithNullPlatform();
    void testCalculateSpeedFromVelocity();
    void testGenerateEntityTooltipWithETA();
    void testShowTooltip();
};

#endif

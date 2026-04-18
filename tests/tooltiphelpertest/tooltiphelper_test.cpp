#include "tooltiphelper_test.h"
#include "GUI/Tacticaldisplay/tooltiphelper.h"
#include "core/Hierarchy/Components/transform.h"
#include "core/Hierarchy/Components/trajectory.h"
#include "core/Hierarchy/Struct/waypoints.h"
#include "core/Hierarchy/Struct/vector.h"
#include "core/Hierarchy/EntityProfiles/platform.h"
#include <QTest>
#include <QVector3D>

// Helper functions (static, not part of the class)
static MeshEntry createMinimalMeshEntry()
{
    MeshEntry entry;
    entry.name = "TestEntity";
    entry.entity = nullptr;
    entry.platform = nullptr;
    entry.coreTransform = new Transform();
    entry.trajectory = new Trajectory();

    Waypoints* wp1 = new Waypoints();
    wp1->position = new Vector(12.34, 567.0, 56.78);
    wp1->speed = 500.0;
    entry.trajectory->Trajectories.push_back(wp1);

    Waypoints* wp2 = new Waypoints();
    wp2->position = new Vector(12.50, 567.0, 56.90);
    wp2->speed = 600.0;
    entry.trajectory->Trajectories.push_back(wp2);

    entry.coreTransform->setGeoCord(12.34, 56.78);
    entry.coreTransform->setAltitude(5000.0);

    return entry;
}

static void cleanupMinimalMeshEntry(MeshEntry& entry)
{
    if (entry.trajectory) {
        for (auto wp : entry.trajectory->Trajectories) {
            delete wp->position;
            delete wp;
        }
        delete entry.trajectory;
        entry.trajectory = nullptr;
    }
    delete entry.coreTransform;
    entry.coreTransform = nullptr;
}

void TestTooltipHelper::testGetAvailableFields()
{
    QStringList fields = TooltipHelper::getAvailableFields();
    QCOMPARE(fields.size(), 14);
    QVERIFY(fields.contains("Name"));
    QVERIFY(fields.contains("Speed"));
    QVERIFY(fields.contains("Move Speed"));
    QVERIFY(fields.contains("Latitude"));
    QVERIFY(fields.contains("Longitude"));
    QVERIFY(fields.contains("Altitude"));
    QVERIFY(fields.contains("Trajectory ETA"));
}

void TestTooltipHelper::testFormatTooltipHTML()
{
    QMap<QString, QString> sampleData;
    sampleData["Name"] = "TestEntity";
    sampleData["Speed"] = "750 km/h";
    sampleData["Latitude"] = "12.340000";
    sampleData["Longitude"] = "56.780000";
    sampleData["Altitude"] = "5000 ft";

    QSet<QString> activeFields = {"Name", "Speed", "Latitude", "Longitude", "Altitude"};
    QString html = TooltipHelper::formatTooltipHTML("TestEntity", sampleData, activeFields);
    QVERIFY(!html.isEmpty());
    QVERIFY(html.contains("TestEntity"));
    QVERIFY(html.contains("750 km/h"));
    QVERIFY(html.contains("12.340000"));
}

void TestTooltipHelper::testFormatCompactTooltip()
{
    QMap<QString, QString> sampleData;
    sampleData["Name"] = "TestEntity";
    sampleData["Speed"] = "750 km/h";
    QString compact = TooltipHelper::formatCompactTooltip("TestEntity", sampleData);
    QVERIFY(!compact.isEmpty());
    QVERIFY(compact.contains("TestEntity"));
    QVERIFY(compact.contains("Speed: 750 km/h"));
}

void TestTooltipHelper::testGetDynamicModelInfoWithNullPlatform()
{
    MeshEntry dummy = createMinimalMeshEntry();
    QMap<QString, QString> info = TooltipHelper::getDynamicModelInfo(dummy);
    QVERIFY(info.isEmpty());
    cleanupMinimalMeshEntry(dummy);
}

void TestTooltipHelper::testCalculateSpeedFromVelocity()
{
    QVector3D* vel = new QVector3D(100, 0, 0);
    QString speedStr = TooltipHelper::calculateSpeedFromVelocity(vel);
    QCOMPARE(speedStr, QString("360.00 km/h"));
    delete vel;
}

void TestTooltipHelper::testGenerateEntityTooltipWithETA()
{
    MeshEntry dummy = createMinimalMeshEntry();
    QSet<QString> activeFields = {"Trajectory ETA"};
    QString tooltip = TooltipHelper::generateEntityTooltip(dummy, activeFields);
    QVERIFY(true); // no crash
    cleanupMinimalMeshEntry(dummy);
}

void TestTooltipHelper::testShowTooltip()
{
    TooltipHelper::showTooltip("Test tooltip", nullptr);
    QVERIFY(true);
}

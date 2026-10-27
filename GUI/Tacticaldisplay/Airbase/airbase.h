#ifndef AIRBASE_H
#define AIRBASE_H

#include <QPainter>
#include <QPixmap>
#include <QString>
#include <vector>
#include <utility>

// Forward declaration to avoid circular includes
class GISlib;

// ============================================================================
// CHAITANYA JAIN CHANGE
//
// Type:
// Safe Refactoring
//
// Objective:
// Extract Airbase functionality from GISlib into a dedicated module.
//
// Root Cause:
// Airbase logic was tightly coupled with GISlib, reducing modularity and
// maintainability.
//
// Legacy Behaviour:
// Airbase rendering and management resided entirely inside GISlib.
//
// New Behaviour:
// GISlib delegates all Airbase responsibilities to the new Airbase module.
// Runtime behavior remains identical.
//
// Why this is safe:
// This is a structural refactor only. No rendering logic, calculations,
// coordinate handling, event flow, or user-visible behavior has been modified.
//
// ============================================================================

class Airbase {
public:
    Airbase();

    void initialize();
    void loadAirbaseData();
    void render(QPainter& painter, GISlib* gis, int zoom);
    void toggleVisibility();

    bool isVisible() const { return showAirbases; }
    const std::vector<std::pair<double, double>>& airbases() const { return airbasePositions; }
    QPixmap getAirbasePixmap() const { return airbasePixmap; }

private:
    std::vector<std::pair<double, double>> airbasePositions;
    bool showAirbases = false;
    QString airbaseIconPath = ":/icons/images/airbase.png";
    QPixmap airbasePixmap;
};

#endif // AIRBASE_H
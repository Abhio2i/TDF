#ifndef ENTITYINFODIALOG_TEST_H
#define ENTITYINFODIALOG_TEST_H

#include <QObject>

class EntityInfoDialog;

class TestEntityInfoDialog : public QObject
{
    Q_OBJECT

private slots:
    void init();
    void cleanup();

    // Existing tests (12)
    void testDialogTitle();
    void testDialogSize();
    void testTitleLabel();
    void testScrollArea();
    void testAttributeTable();
    void testSpeedAltitudeTable();
    void testPositionLabel();
    void testEquipmentButtons();
    void testCheckboxes();
    void testCloseButton();
    void testDataDependentSkipped();

    // New tests (to reach 30+)
    void testAttributeTableColumnWidth();
    void testSpeedAltitudeTableEditTriggers();

    void testCheckboxInitialStates();
    void testClearInfoResetsUI();
    void testSpeedAltitudeSignalExists();
    void testUpdateSignalExists();
    void testFindFormationForEntityWithNull();
    void testAttributeTableRowCount();
    void testSpeedAltitudeTableRowHeight();
    void testEquipmentButtonsTooltips();
    void testOptionsSectionLayout();
    void testDialogModalFlag();
    void testDialogStaysOnTopFlag();
    void testPositionLabelFormat();
    void testAttributeTableItemForeground();
    void testSpeedAltitudeTableCurrentColumnNonEditable();
    void testSpeedAltitudeTableRequestedColumnEditable();
    void testFollowTrajectoryCheckboxToggling();
    void testShowDetectionCheckboxToggling();

private:
    EntityInfoDialog* dialog = nullptr;
};

#endif

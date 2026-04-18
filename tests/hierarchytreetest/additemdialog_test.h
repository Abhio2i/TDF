#ifndef ADDITEMDIALOG_TEST_H
#define ADDITEMDIALOG_TEST_H

#include <QObject>

class AddItemDialog;
class Hierarchy;

class TestAddItemDialog : public QObject
{
    Q_OBJECT

private slots:
    void init();
    void cleanup();

    // Basic properties
    void testWindowTitle();


    // UI elements
    void testNameLineEdit();
    void testOkCancelButtons();

    // Number field (optional)
    void testNumberField();

    // Sensor type (optional)
    void testSensorTypeCombo();

    // Component checkboxes (optional)
    void testComponentCheckboxes();

    // Getters
    void testGetName();
    void testGetNumber();
    void testSetNumber();
    void testGetComponents();

    // Optional: profile combo, team combo, search field, scenario config checkbox
    void testProfileCombo();
    void testTeamCombo();
    void testSearchField();
    void testScenarioConfigCheckbox();

    // Validation
    void testValidationExists();

    // Window flags
    void testWindowFlags();

private:
    AddItemDialog* dialog = nullptr;
    Hierarchy* dummyHierarchyForCleanup = nullptr;
};

#endif

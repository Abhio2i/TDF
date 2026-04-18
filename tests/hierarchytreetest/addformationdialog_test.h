#ifndef ADDFORMATIONDIALOG_TEST_H
#define ADDFORMATIONDIALOG_TEST_H

#include <QObject>

class AddFormationDialog;

class TestAddFormationDialog : public QObject
{
    Q_OBJECT

private slots:
    void init();
    void cleanup();

    // Basic properties
    void testWindowTitle();
    void testIsModal();

    // UI elements
    void testNameLineEdit();
    void testMothershipCombo();
    void testFormationTypeCombo();
    void testAlliesListWidget();
    void testCountLabels();
    void testButtonBox();

    // Getters
    void testGetFormationName();
    void testGetMothershipId();
    void testGetFormationType();
    void testGetAlliesCount();
    void testGetAllies();

    // Behavior
    void testMothershipChangeNoCrash();
    void testValidationExists();
    void testWindowFlags();
    void testDefaultValues();

private:
    AddFormationDialog* dialog = nullptr;
};

#endif

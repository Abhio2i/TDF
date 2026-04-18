#ifndef DOCTRINEPARAMETERS_TEST_H
#define DOCTRINEPARAMETERS_TEST_H

#include <QObject>

class DoctrineParameters;

class TestDoctrineParameters : public QObject
{
    Q_OBJECT

private slots:
    void init();
    void cleanup();

    // UI structure tests
    void testBasicUIElements();
    void testDefaultForceType();
    void testTabSwitching();

    // JSON serialization tests
    void testToJsonStructure();
    void testLoadFromJsonRoundTrip();
    void testResetState();
    void testLegacyFormatLoading();

    // Signal tests
    void testSignalsExist();

    // Default values
    void testDefaultComboValues();

private:
    DoctrineParameters* panel = nullptr;
};

#endif

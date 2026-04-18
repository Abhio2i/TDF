#ifndef FEEDBACK_TEST_H
#define FEEDBACK_TEST_H

#include <QObject>

class Feedback;

class TestFeedbackDialog : public QObject
{
    Q_OBJECT

private slots:
    void init();
    void cleanup();

    // Dialog properties
    void testWindowTitle();
    void testIsModal();
    void testSize();
    void testExists();

    // UI elements
    void testNameLabel();

    void testVersionLabel();
    void testOkButtonExists();
    void testOkButtonProperties();
    void testSeparatorExists();

    // Layout and styling
    void testOkButtonAlignment();
    void testHasStyleSheet();

private:
    Feedback* dialog = nullptr;
};

#endif

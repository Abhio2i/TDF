/* ========================================================================= */
/* File: ProjectInformation.h                                                */
/* Purpose: Defines Feedback dialog for project information and unit tests   */
/* Written by   : Arti Rajpoot                                               */
/* ========================================================================= */
#ifndef PROJECTINFORMATION_H
#define PROJECTINFORMATION_H

#include <QDialog>

class Feedback : public QDialog
{
    Q_OBJECT
public:
    explicit Feedback(QWidget *parent = nullptr);
     // static void runUnitTestsOnce();
};

#endif // PROJECTINFORMATION_H

/* ========================================================================= */
/* File: doctrineareadefinition.h                                          */
/* Purpose: Defines the Doctrine Area Definition panel widget                */
// Written by   : Arti Rajpoot
/* ========================================================================= */

#ifndef DOCTRINEAREADEFINITION_H
#define DOCTRINEAREADEFINITION_H

#include <QWidget>
#include <QLabel>
#include <QVBoxLayout>
#include <QFrame>

// %%% Class Definition %%%
/* Panel widget for displaying doctrine area definition */
class DoctrineAreaDefinition : public QWidget
{
    Q_OBJECT

public:
    explicit DoctrineAreaDefinition(QWidget *parent = nullptr);
    ~DoctrineAreaDefinition() = default;

private:
    // %%% UI Setup Methods %%%
    void setupUI();
    void applyStyles();

    // %%% UI Components %%%
    QLabel *titleLabel = nullptr;
    QFrame *divider = nullptr;
    QLabel *contentLabel = nullptr;
};

#endif // DOCTRINEAREADEFINITION_H

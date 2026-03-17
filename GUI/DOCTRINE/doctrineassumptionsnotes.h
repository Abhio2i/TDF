/* ========================================================================= */
/* File: doctrineassumptionsnotes.h                                        */
/* Purpose: Defines the Doctrine Assumptions / Notes panel widget            */
// Written by   : Arti Rajpoot
/* ========================================================================= */

#ifndef DOCTRINEASSUMPTIONSNOTES_H
#define DOCTRINEASSUMPTIONSNOTES_H

#include <QWidget>
#include <QLabel>
#include <QVBoxLayout>
#include <QFrame>

// %%% Class Definition %%%
/* Panel widget for displaying doctrine assumptions and notes */
class DoctrineAssumptionsNotes : public QWidget
{
    Q_OBJECT

public:
    explicit DoctrineAssumptionsNotes(QWidget *parent = nullptr);
    ~DoctrineAssumptionsNotes() = default;

private:
    // %%% UI Setup Methods %%%
    void setupUI();
    void applyStyles();

    // %%% UI Components %%%
    QLabel *titleLabel = nullptr;
    QFrame *divider = nullptr;
    QLabel *contentLabel = nullptr;
};

#endif // DOCTRINEASSUMPTIONSNOTES_H

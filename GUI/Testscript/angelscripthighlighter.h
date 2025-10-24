/* ========================================================================= */
/* File: angelscripthighlighter.h                                           */
/* Purpose: Defines syntax highlighter for AngelScript code                  */
/* ========================================================================= */

#ifndef ANGELSCRIPTHIGHLIGHTER_H
#define ANGELSCRIPTHIGHLIGHTER_H

#include <QSyntaxHighlighter>                     // For syntax highlighter base
#include <QRegularExpression>                     // For regular expression handling

// %%% Class Definition %%%
/* Syntax highlighter for AngelScript */
class AngelScriptHighlighter : public QSyntaxHighlighter
{
    Q_OBJECT

public:
    // Initialize highlighter
    explicit AngelScriptHighlighter(QTextDocument *parent = nullptr);

protected:
    // Highlight text block
    void highlightBlock(const QString &text) override;

private:
    // %%% Data Structures %%%
    // Highlighting rule structure
    struct HighlightingRule {
        QRegularExpression pattern;               // Pattern to match
        QTextCharFormat format;                   // Format for matched text
    };
    // List of highlighting rules
    QVector<HighlightingRule> highlightingRules;
    // Comment start expression
    QRegularExpression commentStartExpression;
    // Comment end expression
    QRegularExpression commentEndExpression;
    // Format for keywords
    QTextCharFormat keywordFormat;
    // Format for quotations
    QTextCharFormat quotationFormat;
    // Format for functions
    QTextCharFormat functionFormat;
    // Format for single-line comments
    QTextCharFormat singleLineCommentFormat;
    // Format for multi-line comments
    QTextCharFormat multiLineCommentFormat;
    // Format for classes
    QTextCharFormat classFormat;
};

#endif // ANGELSCRIPTHIGHLIGHTER_H

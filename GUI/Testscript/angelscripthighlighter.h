/* =============================================================================
 * FILE:         angelscripthighlighter.h
 * MODULE:       AngelScript Syntax Highlighter
 * PROJECT:      Indigenous Scenario and Sensor Simulation Toolkit (ISSST)
 * ORGANISATION: Oxygen 2 Innovation (O2I).
 * STANDARD:     RTCA DO-178C / ED-12C, DAL B
 * COVERAGE:     Branch / Decision Coverage required (100% true/false paths)
 *
 * DESCRIPTION:  Declares the AngelScriptHighlighter class which provides syntax
 *               highlighting for AngelScript code within QTextDocument. It
 *               highlights keywords, quotations, functions, single‑line comments,
 *               multi‑line comments, and classes using regular expression rules.
 *
 * REQUIREMENTS: REQ-HIGHLIGHT-010  AngelScript syntax highlighter
 *               REQ-HIGHLIGHT-011  Highlight keywords (if, else, for, while, etc.)
 *               REQ-HIGHLIGHT-012  Highlight string literals and quotations
 *               REQ-HIGHLIGHT-013  Highlight function names and class names
 *               REQ-HIGHLIGHT-014  Highlight single‑line and multi‑line comments
 *               REQ-HIGHLIGHT-015  Support for custom highlighting rules
 *
 * AUTHOR:       Arti Rajpoot
 * REVIEWED BY:  [Reviewer Name], [Review Date] — SPR-HIGHLIGHT-001
 *
 *
 * COPYRIGHT:    Oxygen 2 Innovation (O2I). All rights reserved.
 *               Restricted circulation — defence simulation use only.
 * =============================================================================
 */

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
        QRegularExpression pattern;
        QTextCharFormat format;
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

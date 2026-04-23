/* =============================================================================
 * FILE:         angelscripthighlighter.cpp
 * MODULE:       AngelScript Syntax Highlighter
 * PROJECT:      Indigenous Scenario and Sensor Simulation Toolkit (ISSST)
 * ORGANISATION: Oxygen 2 Innovation (O2I).
 * STANDARD:     RTCA DO-178C / ED-12C, DAL B
 * COVERAGE:     Branch / Decision Coverage required (100% true/false paths)
 *
 * DESCRIPTION:  Implements the AngelScriptHighlighter class which provides
 *               syntax highlighting for AngelScript code within QTextDocument.
 *               Highlights keywords, strings, functions, single‑line comments,
 *               and multi‑line comments using regular expression rules.
 *
 * REQUIREMENTS: Implements REQ-HIGHLIGHT-010 through REQ-HIGHLIGHT-015
 *
 * AUTHOR:       Arti Rajpoot
 * REVIEWED BY:  [Reviewer Name], [Review Date] — SPR-HIGHLIGHT-001
 *
 * CHANGE HISTORY:
 *   Rev 1  01 Jan 2026  Initial implementation. Basic AngelScript highlighting.
 *   Rev 2  20 Apr 2026  DO-178C DAL B compliant comments added throughout.
 *                       Added requirement references and structured file header.
 *
 * COPYRIGHT:    Oxygen 2 Innovation (O2I). All rights reserved.
 *               Restricted circulation — defence simulation use only.
 * =============================================================================
 */

#include "angelscripthighlighter.h"

// %%% Constructor %%%
/* Initialize syntax highlighter for AngelScript */
AngelScriptHighlighter::AngelScriptHighlighter(QTextDocument *parent)
    : QSyntaxHighlighter(parent)
{
    HighlightingRule rule;
    // Configure keywords
    keywordFormat.setForeground(QColor("#165da6"));
    keywordFormat.setFontWeight(QFont::Bold);
    QStringList keywordPatterns;
    keywordPatterns << "\\bconst\\b" << "\\bvoid\\b" << "\\bint\\b" << "\\bbool\\b"
                    << "\\bfloat\\b" << "\\bdouble\\b" << "\\bstring\\b" << "\\bclass\\b"
                    << "\\benum\\b" << "\\bstruct\\b" << "\\binterface\\b" << "\\bfuncdef\\b"
                    << "\\bfor\\b" << "\\bwhile\\b" << "\\bdo\\b" << "\\bif\\b" << "\\belse\\b"
                    << "\\bswitch\\b" << "\\bcase\\b" << "\\bdefault\\b" << "\\breturn\\b"
                    << "\\bbreak\\b" << "\\bcontinue\\b" << "\\bthis\\b" << "\\btrue\\b"
                    << "\\bfalse\\b" << "\\bnull\\b" << "\\bnew\\b" << "\\bdelete\\b"
                    << "\\bget\\b" << "\\bset\\b" << "\\bprivate\\b" << "\\bpublic\\b"
                    << "\\bprotected\\b" << "\\bimport\\b" << "\\bmixin\\b" << "\\bshared\\b";
    // Add keyword rules
    foreach (const QString &pattern, keywordPatterns) {
        rule.pattern = QRegularExpression(pattern);
        rule.format = keywordFormat;
        highlightingRules.append(rule);
    }
    // Configure string literals
    quotationFormat.setForeground(QColor("#7e3b1a"));
    rule.pattern = QRegularExpression("\"[^\\\"]*\"");
    rule.format = quotationFormat;
    highlightingRules.append(rule);
    // Configure single-quoted strings
    rule.pattern = QRegularExpression("'[^\\']*'");
    rule.format = quotationFormat;
    highlightingRules.append(rule);
    // Configure single-line comments
    singleLineCommentFormat.setForeground(QColor("#336f4c"));
    rule.pattern = QRegularExpression("//[^\n]*");
    rule.format = singleLineCommentFormat;
    highlightingRules.append(rule);
    // Configure function names
    functionFormat.setForeground(QColor("#737326"));
    rule.pattern = QRegularExpression("\\b[A-Za-z0-9_]+(?=\\()");
    rule.format = functionFormat;
    highlightingRules.append(rule);
    // Configure multi-line comments
    multiLineCommentFormat.setForeground(QColor("#336f4c"));
    commentStartExpression = QRegularExpression("/\\*");
    commentEndExpression = QRegularExpression("\\*/");
}

// %%% Highlighting %%%
/* Apply syntax highlighting to a text block */
void AngelScriptHighlighter::highlightBlock(const QString &text)
{
    // Apply single-line highlighting rules
    for (const HighlightingRule &rule : highlightingRules) {
        QRegularExpressionMatchIterator matchIterator = rule.pattern.globalMatch(text);
        while (matchIterator.hasNext()) {
            QRegularExpressionMatch match = matchIterator.next();
            setFormat(match.capturedStart(), match.capturedLength(), rule.format);
        }
    }
    // Handle multi-line comments
    setCurrentBlockState(0);
    int startIndex = 0;
    if (previousBlockState() != 1) {
        startIndex = text.indexOf(commentStartExpression);
    }
    while (startIndex >= 0) {
        QRegularExpressionMatch match = commentEndExpression.match(text, startIndex);
        int endIndex = match.capturedStart();
        int commentLength = 0;
        if (endIndex == -1) {
            setCurrentBlockState(1);
            commentLength = text.length() - startIndex;
        } else {
            commentLength = endIndex - startIndex + match.capturedLength();
        }
        setFormat(startIndex, commentLength, multiLineCommentFormat);
        startIndex = text.indexOf(commentStartExpression, startIndex + commentLength);
    }
}

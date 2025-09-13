#ifndef ANGELSCRIPTHIGHLIGHTER_H
#define ANGELSCRIPTHIGHLIGHTER_H

#include <QSyntaxHighlighter>
#include <QRegularExpression>

class AngelScriptHighlighter : public QSyntaxHighlighter
{
    Q_OBJECT

public:
    AngelScriptHighlighter(QTextDocument *parent = nullptr);

protected:
    void highlightBlock(const QString &text) override;

private:
    struct HighlightingRule {
        QRegularExpression pattern;
        QTextCharFormat format;
    };
    QVector<HighlightingRule> highlightingRules;

    QRegularExpression commentStartExpression;
    QRegularExpression commentEndExpression;

    QTextCharFormat keywordFormat;
    QTextCharFormat quotationFormat;
    QTextCharFormat functionFormat;
    QTextCharFormat singleLineCommentFormat;
    QTextCharFormat multiLineCommentFormat;
    QTextCharFormat classFormat;
};

#endif // ANGELSCRIPTHIGHLIGHTER_H

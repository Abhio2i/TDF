
#ifndef TESTSCRIPTDIALOG_H
#define TESTSCRIPTDIALOG_H
#include "GUI/Testscript/angelscripthighlighter.h"
#include <QWidget>
#include <QTextEdit>
#include <QPushButton>
#include <QLineEdit>
#include <QComboBox>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QLabel>
#include <QDir>
#include <QMessageBox>
#include <QCoreApplication>
#include <QCompleter>
#include <QStringListModel>
#include <QPainter>

class TestScriptDialog : public QWidget
{
    Q_OBJECT
public:
    explicit TestScriptDialog(QWidget *parent = nullptr, bool editMode = false, const QString &filePath = QString());
    ~TestScriptDialog();

    class LineNumberArea : public QWidget
    {
    public:
        LineNumberArea(QTextEdit *editor);
        int lineNumberAreaWidth() const;
    protected:
        void paintEvent(QPaintEvent *event) override;
    private:
        QTextEdit *codeEditor;
    };

signals:
    void runScriptstring(QString code);
    void closed();
private slots:
    void onRunButtonClicked();
    void onBrowseButtonClicked();
    void onOkButtonClicked();
    void onCancelButtonClicked();
    void onScriptPathChanged();
    void onScriptNameChanged(const QString &text);
    void insertCompletion(const QString &completion);
    void handleTextChanged();
    void updateLineNumberArea();
protected:
    void keyPressEvent(QKeyEvent *event) override;
private:
    QTextEdit *codeEditor;
    LineNumberArea *lineNumberArea;
    QPushButton *runButton;
    QPushButton *browseButton;
    QPushButton *okButton;
    QPushButton *cancelButton;
    QComboBox *scriptNameCombo;
    QComboBox *scriptTypeCombo;
    QLineEdit *scriptPathEdit;
    QLabel *scriptNameLabel;
    QLabel *scriptTypeLabel;
    QLabel *scriptPathLabel;
    QString editFilePath;
    bool isEditMode;
    AngelScriptHighlighter *highlighter;
    QCompleter *completer;
    QStringListModel *wordListModel;
};
#endif // TESTSCRIPTDIALOG_H

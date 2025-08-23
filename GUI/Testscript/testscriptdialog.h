
#ifndef TESTSCRIPTDIALOG_H
#define TESTSCRIPTDIALOG_H

#include <QDialog>
#include <QTextEdit>
#include <QPushButton>
#include <QLineEdit>
#include <QComboBox>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <qlabel.h>

class TestScriptDialog : public QDialog
{
    Q_OBJECT

public:
    explicit TestScriptDialog(QWidget *parent = nullptr, bool editMode = false, const QString &filePath = QString());
    ~TestScriptDialog();

private slots:
    void onRunButtonClicked();
    void onBrowseButtonClicked();
    void onOkButtonClicked();
    void onCancelButtonClicked();
    void onScriptPathChanged();

private:
    QTextEdit *codeEditor;
    QPushButton *runButton;
    QPushButton *browseButton;
    QPushButton *okButton;
    QPushButton *cancelButton;
    QLineEdit *scriptNameEdit;
    QComboBox *scriptTypeCombo;
    QLineEdit *scriptPathEdit;
    QLabel *scriptNameLabel;
    QLabel *scriptTypeLabel;
    QLabel *scriptPathLabel;
    QString editFilePath; // Store file path for edit mode
    bool isEditMode; // Track edit mode
};

#endif // TESTSCRIPTDIALOG_H

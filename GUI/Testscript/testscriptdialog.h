#ifndef TESTSCRIPTDIALOG_H
#define TESTSCRIPTDIALOG_H
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

class TestScriptDialog : public QWidget
{
    Q_OBJECT
public:
    explicit TestScriptDialog(QWidget *parent = nullptr, bool editMode = false, const QString &filePath = QString());
    ~TestScriptDialog();
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
private:
    QTextEdit *codeEditor;
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
};
#endif // TESTSCRIPTDIALOG_H

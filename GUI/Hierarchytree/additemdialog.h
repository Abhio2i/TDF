
#ifndef ADDITEMDIALOG_H
#define ADDITEMDIALOG_H

#include <QDialog>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QCheckBox>
#include <QDialogButtonBox>
#include <QGroupBox>
#include <QMap>

class AddItemDialog : public QDialog
{
    Q_OBJECT
public:

    enum DialogType {
        EntityType,
        Folder
    };

    int getNumber() const;
    QStringList getNames() const;
    explicit AddItemDialog(DialogType type, const QString &specificType = "", QWidget *parent = nullptr);
    QString getName() const;
    QVariantMap getComponents() const;
    static QMap<QString, bool> defaultComponents();

private:

    void setupUI(DialogType type);
    QLineEdit *nameLineEdit;
    QLineEdit* numberLineEdit;
    QMap<QString, QCheckBox*> componentCheckboxes;
    QString specificType;
};

#endif // ADDITEMDIALOG_H


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

    explicit AddItemDialog(DialogType type, QWidget *parent = nullptr);

    QString getName() const;
    QVariantMap getComponents() const;

    // Static method to manage component names and default states
    static QMap<QString, bool> defaultComponents();

private:
    void setupUI(DialogType type);

    QLineEdit *nameLineEdit;
    QMap<QString, QCheckBox*> componentCheckboxes;
};

#endif // ADDITEMDIALOG_H

// Header guard to prevent multiple inclusions
#ifndef ADDITEMDIALOG_H
#define ADDITEMDIALOG_H

// Include necessary Qt classes for dialog and UI components
#include <QDialog>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QCheckBox>
#include <QDialogButtonBox>
#include <QGroupBox>
#include <QMap>

// Declaration of AddItemDialog class, inheriting from QDialog
class AddItemDialog : public QDialog
{
    // Macro to declare Qt meta-object compiler features (signals/slots)
    Q_OBJECT

public:
    // Enum to define dialog types (EntityType or Folder)
    enum DialogType {
        EntityType,
        Folder
    };

    // Constructor with dialog type and optional parent widget
    explicit AddItemDialog(DialogType type, QWidget *parent = nullptr);

    // Getter method to retrieve the name entered in the dialog
    QString getName() const;

    // Getter method to retrieve the selected components as a QVariantMap
    QVariantMap getComponents() const;

    // Static method to return default component settings
    static QMap<QString, bool> defaultComponents();

private:
    // Private method to set up the dialog's user interface based on type
    void setupUI(DialogType type);

    // Private member to store the name input field
    QLineEdit *nameLineEdit;

    // Private member to store checkboxes for components
    QMap<QString, QCheckBox*> componentCheckboxes;
};

// End of header guard
#endif // ADDITEMDIALOG_H

/* ========================================================================= */
/* File: additemdialog.h                                                    */
/* Purpose: Defines dialog for adding items to hierarchy                     */
/* ========================================================================= */

#ifndef ADDITEMDIALOG_H
#define ADDITEMDIALOG_H

#include <QDialog>                                // For dialog base class
#include <QVBoxLayout>                            // For vertical layout
#include <QHBoxLayout>                            // For horizontal layout
#include <QLabel>                                 // For label widget
#include <QLineEdit>                              // For text input widget
#include <QCheckBox>                              // For checkbox widget
#include <QDialogButtonBox>                       // For dialog buttons
#include <QGroupBox>                              // For group box widget
#include <QMap>                                   // For key-value mapping

// %%% Class Definition %%%
/* Dialog for adding entities or folders */
class AddItemDialog : public QDialog
{
    Q_OBJECT

public:
    // Enum for dialog types
    enum DialogType {
        EntityType,
        Folder
    };

    // Get item number
    int getNumber() const;
    // Get item names
    QStringList getNames() const;
    // Initialize dialog
    explicit AddItemDialog(DialogType type, const QString &specificType = "", QWidget *parent = nullptr);
    // Get item name
    QString getName() const;
    // Get selected components
    QVariantMap getComponents() const;
    // Get default components
    static QMap<QString, bool> defaultComponents();

private:
    // %%% UI Setup Methods %%%
    // Configure dialog UI
    void setupUI(DialogType type);

    // %%% UI Components %%%
    // Name input field
    QLineEdit *nameLineEdit;
    // Number input field
    QLineEdit* numberLineEdit;
    // Component checkboxes
    QMap<QString, QCheckBox*> componentCheckboxes;
    // Specific item type
    QString specificType;
};

#endif // ADDITEMDIALOG_H

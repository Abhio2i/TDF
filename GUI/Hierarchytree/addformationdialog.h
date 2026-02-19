

// #endif // ADDFORMATIONDIALOG_H
#ifndef ADDFORMATIONDIALOG_H
#define ADDFORMATIONDIALOG_H

#include <QDialog>
#include <QVariantMap>
#include <QList>
#include <QListWidget>
#include <QLineEdit>
#include <QComboBox>
#include <QLabel>
#include <QGroupBox>
#include <QFormLayout>
#include <QVBoxLayout>
#include <QDialogButtonBox>
#include <QPushButton>

class AddFormationDialog : public QDialog
{
    Q_OBJECT

public:
    explicit AddFormationDialog(const QList<QVariantMap>& selectedEntities,
                                QWidget *parent = nullptr);
    ~AddFormationDialog();

    // Get user inputs
    QString getFormationName() const;
    QString getMothershipId() const;
    QString getFormationType() const;
    QList<QVariantMap> getAllies() const;
    int getAlliesCount() const;

private slots:
    void onMothershipChanged(int index);
    void accept() override;

private:
    void setupUI();
    void updateAlliesList();

    QList<QVariantMap> m_selectedEntities;
    QList<QVariantMap> m_allies;

    // UI Components
    QListWidget *m_listWidgetAllies;
    QLineEdit *m_lineEditFormationName;
    QComboBox *m_comboBoxMothership;
    QComboBox *m_comboBoxFormationType;
    QLabel *m_labelAlliesCount;
    QLabel *m_labelSelectedCount;
};

#endif // ADDFORMATIONDIALOG_H

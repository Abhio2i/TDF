//======================Waris===========================
#include "addformationdialog.h"
#include <QMessageBox>
#include <QHBoxLayout>
#include "tests/hierarchytreetest/addformationdialog_test.h"
#include "GUI/mainwindow.h"
#include "addformationdialog.h"
#include <QMessageBox>
#include <QHBoxLayout>
AddFormationDialog::AddFormationDialog(const QList<QVariantMap>& selectedEntities,
                                       QWidget *parent)
    : QDialog(parent)
    , m_selectedEntities(selectedEntities)
{
    setWindowTitle("Create Formation");
    setModal(true);
    setMinimumSize(500, 400);
    setStyleSheet(
        "QDialog { background-color: #0F2636; color: white; border: 2px solid #27446d; }"
        "QLabel { color: white; }"
        "QLineEdit { color: white; background-color: #1A3652; border: 1px solid #27446d; padding: 4px; }"
        "QComboBox { color: white; background-color: #1A3652; border: 1px solid #27446d; padding: 4px; }"
        "QComboBox QAbstractItemView { background-color: #1A3652; color: white; selection-background-color: #27446d; }"
        "QGroupBox { color: white; border: 1px solid #27446d; margin-top: 8px; padding-top: 8px; }"
        "QGroupBox::title { color: white; subcontrol-origin: margin; left: 8px; }"
        "QListWidget { color: white; background-color: #1A3652; border: 1px solid #27446d; }"
        "QPushButton { color: white; background-color: #1A3652; border: 1px solid #27446d; padding: 6px 14px; border-radius: 3px; }"
        "QPushButton:hover { background-color: #27446d; }"
        );
    setupUI();
    int alliesCount = selectedEntities.size() - 1;
    m_labelAlliesCount->setText(QString::number(alliesCount));
    updateAlliesList();

}

AddFormationDialog::~AddFormationDialog()
{
}

void AddFormationDialog::setupUI()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    // Form Layout for inputs
    QFormLayout *formLayout = new QFormLayout();

    // Show selected entities count at the top
    m_labelSelectedCount = new QLabel(QString::number(m_selectedEntities.size()));
    formLayout->addRow("Selected Entities:", m_labelSelectedCount);

    // Formation Name
    m_lineEditFormationName = new QLineEdit();
    m_lineEditFormationName->setText("Formation_" + QString::number(m_selectedEntities.size()));
    formLayout->addRow("Formation Name:", m_lineEditFormationName);

    // Mothership ComboBox
    m_comboBoxMothership = new QComboBox();
    for (const auto& entity : m_selectedEntities) {
        QString name = entity["name"].toString();
        QString id = entity["ID"].toString();
        QString type = entity["type"].toString();

        if (type == "entity") {
            m_comboBoxMothership->addItem(name, id);
        }
    }
    formLayout->addRow("Mothership:", m_comboBoxMothership);

    // Formation Type ComboBox
    m_comboBoxFormationType = new QComboBox();
    QStringList formationTypes = {
        "Line", "V", "Diamond", "Square", "Column",
        "Echelon Left", "Echelon Right", "Staggered Column", "Wedge"
    };
    m_comboBoxFormationType->addItems(formationTypes);
    m_comboBoxFormationType->setCurrentText("V");
    formLayout->addRow("Formation Type:", m_comboBoxFormationType);

    // Allies Count
    m_labelAlliesCount = new QLabel("0");
    formLayout->addRow("Allies Count:", m_labelAlliesCount);

    mainLayout->addLayout(formLayout);

    // Allies Group
    QGroupBox *alliesGroup = new QGroupBox("Allies");
    QVBoxLayout *alliesLayout = new QVBoxLayout();
    m_listWidgetAllies = new QListWidget();
    m_listWidgetAllies->setSelectionMode(QAbstractItemView::NoSelection);
    alliesLayout->addWidget(m_listWidgetAllies);
    alliesGroup->setLayout(alliesLayout);
    mainLayout->addWidget(alliesGroup);

    // Button Box
    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok |
                                                       QDialogButtonBox::Cancel);
    mainLayout->addWidget(buttonBox);

    // Connect signals
    connect(buttonBox, &QDialogButtonBox::accepted, this, &AddFormationDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &AddFormationDialog::reject);
    connect(m_comboBoxMothership, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &AddFormationDialog::onMothershipChanged);
}

QString AddFormationDialog::getFormationName() const
{
    return m_lineEditFormationName->text().trimmed();
}

QString AddFormationDialog::getMothershipId() const
{
    return m_comboBoxMothership->currentData().toString();
}

QString AddFormationDialog::getFormationType() const
{
    return m_comboBoxFormationType->currentText();
}

QList<QVariantMap> AddFormationDialog::getAllies() const
{
    return m_allies;
}

int AddFormationDialog::getAlliesCount() const
{
    return m_allies.size();
}

void AddFormationDialog::onMothershipChanged(int index)
{
    if (index >= 0) {
        updateAlliesList();
    }
}

void AddFormationDialog::updateAlliesList()
{
    m_allies.clear();
    m_listWidgetAllies->clear();
    QString mothershipId = m_comboBoxMothership->currentData().toString();
    for (int i = 0; i < m_selectedEntities.size(); ++i) {
        QString id = m_selectedEntities[i]["ID"].toString();
        if (id != mothershipId) {
            m_allies.append(m_selectedEntities[i]);
            QString name = m_selectedEntities[i]["name"].toString();
            m_listWidgetAllies->addItem(name + " (" + id + ")");
        }
    }
    m_labelAlliesCount->setText(QString::number(m_allies.size()));
}
void AddFormationDialog::accept()
{
    if (getFormationName().isEmpty()) {
        QMessageBox::warning(this, "Invalid Input",
                             "Formation name cannot be empty.");
        return;
    }
    if (getAlliesCount() < 1) {
        QMessageBox::warning(this, "Invalid Formation",
                             "A formation needs at least 1 ally.");
        return;
    }
    QDialog::accept();
}


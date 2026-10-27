//======================Waris===========================
#include "addformationdialog.h"
#include <QMessageBox>
#include <QHBoxLayout>
#include "tests/hierarchytreetest/addformationdialog_test.h"
#include "GUI/mainwindow.h"
#include "addformationdialog.h"
#include <QMessageBox>
#include <QHBoxLayout>
#include "GUI/dialogwindowutils.h"


AddFormationDialog::AddFormationDialog(const QList<QVariantMap>& selectedEntities,
                                       QWidget *parent)
    : QDialog(parent)
    , m_selectedEntities(selectedEntities)
{
    setWindowTitle("Create Formation");
    setModal(true);
    setMinimumSize(500, 400);
    makeDialogResizable(this);
    setWindowModality(Qt::WindowModal);
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

    QFormLayout *formLayout = new QFormLayout();

    // Selected count
    m_labelSelectedCount = new QLabel(QString::number(m_selectedEntities.size()));
    formLayout->addRow("Selected Entities:", m_labelSelectedCount);

    // Formation Name
    m_lineEditFormationName = new QLineEdit();
    m_lineEditFormationName->setText("Formation_" + QString::number(m_selectedEntities.size()));
    formLayout->addRow("Formation Name:", m_lineEditFormationName);

    // Mothership
    m_comboBoxMothership = new QComboBox();
    for (const auto& entity : m_selectedEntities) {
        QString name = entity["name"].toString();
        QString id   = entity["ID"].toString();
        QString type = entity["type"].toString();
        if (type == "entity") {
            m_comboBoxMothership->addItem(name, id);
        }
    }
    formLayout->addRow("Mothership:", m_comboBoxMothership);

    // Formation Type
    m_comboBoxFormationType = new QComboBox();
    QStringList formationTypes = {
        "Line", "V", "Diamond", "Square", "Column",
        "Echelon Left", "Echelon Right", "Staggered Column", "Wedge",
        "Coil", "Herringbone", "Skirmish", "Box"
    };
    m_comboBoxFormationType->addItems(formationTypes);
    m_comboBoxFormationType->setCurrentText("V");
    formLayout->addRow("Formation Type:", m_comboBoxFormationType);

    // Allies Count
    m_labelAlliesCount = new QLabel("0");
    formLayout->addRow("Allies Count:", m_labelAlliesCount);

    // ── Custom from Database (yahan pehle add karo) ─────────────────────
    m_chkUseCustom = new QCheckBox("Use Custom Formation from Database");
    // ARTI CHANGE: explicit indicator styling — default checkbox box/tick was invisible against the dark theme
    m_chkUseCustom->setStyleSheet(
        "QCheckBox { color: white; spacing: 6px; }"
        "QCheckBox::indicator { width: 16px; height: 16px; border: 1px solid #4A90D9; border-radius: 2px; background-color: #1A3652; }"
        "QCheckBox::indicator:hover { border: 1px solid #6FB1FF; }"
        "QCheckBox::indicator:checked { border: 1px solid #6FB1FF; image: url(:/icons/images/check-box.png); }"
        );
    formLayout->addRow("", m_chkUseCustom);

    m_comboCustomFormations = new QComboBox();
    m_comboCustomFormations->setEnabled(false);
    formLayout->addRow("Custom Formation:", m_comboCustomFormations);

       QWidget *formationTypeLabel = formLayout->labelForField(m_comboBoxFormationType);

    connect(m_chkUseCustom, &QCheckBox::toggled, this, [=](bool checked) {
        m_comboCustomFormations->setEnabled(checked);
        m_comboBoxFormationType->setEnabled(!checked);
        // ARTI CHANGE: manually hide/show both the label and the combo to fully hide the row
        m_comboBoxFormationType->setVisible(!checked);
        if (formationTypeLabel)
            formationTypeLabel->setVisible(!checked);
        if (checked) {
            loadCustomFormationsFromDatabase();   // har tick pe refresh
        }
    });

    mainLayout->addLayout(formLayout);

    // Allies Group
    QGroupBox *alliesGroup = new QGroupBox("Allies");
    QVBoxLayout *alliesLayout = new QVBoxLayout();
    m_listWidgetAllies = new QListWidget();
    m_listWidgetAllies->setSelectionMode(QAbstractItemView::NoSelection);
    alliesLayout->addWidget(m_listWidgetAllies);
    alliesGroup->setLayout(alliesLayout);
    mainLayout->addWidget(alliesGroup);

    // Buttons
    QDialogButtonBox *buttonBox = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    mainLayout->addWidget(buttonBox);

    connect(buttonBox, &QDialogButtonBox::accepted, this, &AddFormationDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &AddFormationDialog::reject);
    connect(m_comboBoxMothership, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &AddFormationDialog::onMothershipChanged);

    // pehli baar load
    loadCustomFormationsFromDatabase();
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
void AddFormationDialog::loadCustomFormationsFromDatabase()
{
    if (!m_comboCustomFormations)
        return;

    m_comboCustomFormations->clear();
    m_comboCustomFormations->addItem("— Select Custom Formation —", QString());

    MainWindow *mw = MainWindow::instance();
    if (!mw) {
        qWarning() << "[AddFormationDialog] MainWindow is null";
        return;
    }

    Hierarchy *dbHier = nullptr;
    if (mw->databaseEditor && mw->databaseEditor->hierarchy)
        dbHier = mw->databaseEditor->hierarchy;
    else
        dbHier = mw->getDatabaseHierarchy();

    if (!dbHier) {
        qWarning() << "[AddFormationDialog] Database hierarchy not available";
        m_comboCustomFormations->addItem("(Database not loaded)", QString());
        return;
    }

    int found = 0;

    for (const auto &pair : dbHier->Entities) {
        Entity *ent = pair.second;
        if (!ent)
            continue;

        // ── Formation detect ──────────────────────────────────────────
        bool isFormation = false;

        if (ent->type == Constants::EntityType::Formation)
            isFormation = true;

        if (!isFormation && dynamic_cast<Formation*>(ent) != nullptr)
            isFormation = true;

        QJsonObject j = ent->toJson();

        if (!isFormation) {
            if (j.contains("type")) {
                if (j["type"].isObject()) {
                    QString val = j["type"].toObject().value("value").toString();
                    if (val.compare("Formation", Qt::CaseInsensitive) == 0)
                        isFormation = true;
                } else if (j["type"].isString()) {
                    if (j["type"].toString().compare("Formation", Qt::CaseInsensitive) == 0)
                        isFormation = true;
                }
            }
            if (!isFormation) {
                QString n = QString::fromStdString(ent->Name);
                if (n.compare("Formation", Qt::CaseInsensitive) == 0)
                    isFormation = true;
            }
        }

        if (!isFormation)
            continue;

        // ── Positions count (Offset structure support) ────────────────
        int pointCount = 0;
        QJsonArray raw;

        if (j.contains("formationPositions") && j["formationPositions"].isArray())
            raw = j["formationPositions"].toArray();
        else if (j.contains("FormationPositions") && j["FormationPositions"].isArray())
            raw = j["FormationPositions"].toArray();

        for (const QJsonValue& v : raw) {
            QJsonObject posObj = v.toObject();

            // Format A: { positionName, Offset: {x,y,z} }
            if (posObj.contains("Offset") && posObj["Offset"].isObject()) {
                pointCount++;
            }
            // Format B: simple {x, y}
            else if (posObj.contains("x") || posObj.contains("y")) {
                pointCount++;
            }
            // Format C: full FormationPosition with entity – still a slot
            else if (posObj.contains("positionName")) {
                pointCount++;
            }
        }

        // Memory se bhi count (agar toJson empty ho lekin slots hon)
        if (pointCount == 0) {
            Formation* form = dynamic_cast<Formation*>(ent);
            if (form && form->formationPositions) {
                pointCount = static_cast<int>(form->formationPositions->size());
            }
        }

        QString name = QString::fromStdString(ent->Name);
        QString id   = QString::fromStdString(ent->ID);

        QString display = name;
        if (pointCount > 0)
            display += QString("  (%1 pts)").arg(pointCount);
        else
            display += "  (no points yet)";

        m_comboCustomFormations->addItem(display, id);
        found++;
    }

    qDebug() << "[AddFormationDialog] Formations found in DB:" << found;

    if (found == 0) {
        m_comboCustomFormations->addItem("(No Formation entities in Database)", QString());
    }
}
QString AddFormationDialog::getSelectedCustomFormationId() const
{
    if (m_chkUseCustom && m_chkUseCustom->isChecked())
        return m_comboCustomFormations->currentData().toString();
    return QString();
}

bool AddFormationDialog::isCustomFromDatabase() const
{
    return m_chkUseCustom && m_chkUseCustom->isChecked()
           && !getSelectedCustomFormationId().isEmpty();
}

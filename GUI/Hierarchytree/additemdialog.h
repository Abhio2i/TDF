#ifndef ADDITEMDIALOG_H
#define ADDITEMDIALOG_H

#include "GUI/Hierarchytree/customtrajectorydialog.h"
#include <QDialog>
#include <QVariantMap>
#include <QHBoxLayout>
#include <QCompleter>
#include <QStringListModel>

class QLineEdit;
class QComboBox;
class QCheckBox;
class QGroupBox;
class QSpinBox;
class QPushButton;
class Hierarchy;

/* Dialog window for adding entities/folders with configurable properties */
class AddItemDialog : public QDialog
{
    Q_OBJECT

public:
    QString getSelectedEntityId() const;

    enum DialogType {
        EntityType,
        FolderType
    };

    enum DialogMode {
        NormalMode,
        ComponentSensorMode,
        ComponentIFFMode,
        ComponentRadioMode,
        ComponentWeaponMode
    };

    QVariantMap getEntityComponents() const;

    AddItemDialog(DialogType type,
                  const QString &specificType,
                  DialogMode dialogMode,
                  Hierarchy* hierarchy,
                  QWidget *parent = nullptr,
                  const QString& editorContext = "");

    CustomTrajectoryDialog customDialog;

    QString getName() const;
    void setNumber(int value);
    int getNumber() const;
    QVariantMap getComponents() const;
    QString getSensorType() const;
    QString getProfileId() const;
    QString getProfileName() const;
    QString getComponentType() const;
    QString getRadioType() const;
    QString getIFFType() const;



    bool isScenarioconfigEnabled() const;
    QString getScType() const;
    QPointF getCity() const;
    int getScRange() const;
    int getMinRadioRange() const;
    int getMaxRadioRange() const;
    int getMinRadarRange() const;
    int getMaxRadarRange() const;
    QString getTrajectory() const;
    int getMinPlaneSpeed() const;
    int getMaxPlaneSpeed() const;
    int getMinTurnRadius() const;
    int getMaxTurnRadius() const;
    bool isScEnabled() const { return isScenarioconfigEnabled(); }
    int getRange() const { return getScRange(); }

    bool isScSensorsEnabled() const;
    bool isScIffEnabled() const;
    bool isScRadioEnabled() const;

    void populateEntityProfiles(const QString &profileTypeFilter = "",
                                const QString &categoryFilter = "All");
    QString getSelectedTeam() const;

private slots:
    void onScCheckBoxStateChanged(int state);

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;

private:
    QString m_editorContext;
    QString selectedEntityId;
    QVariantMap entityComponents;

    void setupUI(DialogType type);
    void setupScSection();
    bool detectDatabaseEditorFromWindow();
    bool validateInputs();

    QHBoxLayout* createMinMaxSpinBoxPair(const QString& label,
                                         QSpinBox*& minSpinBox,
                                         QSpinBox*& maxSpinBox,
                                         int minDefault,
                                         int maxDefault,
                                         int minRange,
                                         int maxRange,
                                         QString unit = " m");

    void populateSensorProfiles();
    void populateIFFProfiles();
    void populateRadioProfiles();
    void populateWeaponProfiles();
    void populateProfiles(const QString& profileType);

    QLineEdit *nameLineEdit;
    QLineEdit *numberLineEdit;
    QComboBox *sensorTypeComboBox;
    QComboBox *radioTypeComboBox;
    QComboBox *iffTypeComboBox;

    QComboBox *profileComboBox;

    QCheckBox *scCheckBox;
    QGroupBox *scOptionsGroup;
    QWidget   *scComponentsWidget = nullptr;
    QComboBox *scTypeComboBox;
    QComboBox *cityComboBox;
    QLineEdit *rangeLineEdit;
    QCheckBox *scSensorsCheckBox = nullptr;
    QCheckBox *scIffCheckBox     = nullptr;
    QCheckBox *scRadioCheckBox   = nullptr;
    QSpinBox *minRadioRangeSpinBox;
    QSpinBox *maxRadioRangeSpinBox;
    QSpinBox *minRadarRangeSpinBox;
    QSpinBox *maxRadarRangeSpinBox;
    QComboBox *trajectoryComboBox;
    QSpinBox *minPlaneSpeedSpinBox;
    QSpinBox *maxPlaneSpeedSpinBox;
    QSpinBox *minTurnRadiusSpinBox;
    QSpinBox *maxTurnRadiusSpinBox;

    QMap<QString, QCheckBox*> componentCheckboxes;

    QString specificType;
    DialogMode m_dialogMode;
    Hierarchy* m_hierarchy;

    void populateComponentsFromEntity(const QString& entityId, const QString& profileName);

    QLineEdit *entitySearchLineEdit;
    QComboBox *profileFilterComboBox;
    QCompleter *entityCompleter;
    QMap<QString, QVariantList> entityMap;
    void clearEntitySelection();
    QString m_profileContext;
    QString determineProfileContext(const QString& specificType,
                                    DialogMode dialogMode,
                                    const QString& editorContext);
    QComboBox *teamSelectComboBox = nullptr;
    bool        m_addNewMode            = false;
    QWidget*    m_entitySearchContainer = nullptr;
    QPushButton* m_addNewBtn            = nullptr;
    QWidget* m_sensorTypeContainer = nullptr;
    QWidget* m_radioTypeContainer = nullptr;
    QWidget* m_iffTypeContainer = nullptr;

};

#endif // ADDITEMDIALOG_H

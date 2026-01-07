
#ifndef ADDITEMDIALOG_H
#define ADDITEMDIALOG_H

#include <QDialog>
#include <QVariantMap>
#include <QHBoxLayout>

class QLineEdit;
class QComboBox;
class QCheckBox;
class QGroupBox;
class QSpinBox;
class Hierarchy;

class AddItemDialog : public QDialog
{
    Q_OBJECT

public:
    enum DialogType {
        EntityType,
        FolderType
    };

    enum DialogMode {
        NormalMode,
        ComponentSensorMode,
        ComponentIFFMode,
        ComponentRadioMode
    };

    AddItemDialog(DialogType type,
                  const QString &specificType = QString(),
                  DialogMode dialogMode = NormalMode,
                  Hierarchy* hierarchy = nullptr,
                  QWidget *parent = nullptr);

    QString getName() const;
    int getNumber() const;
    QVariantMap getComponents() const;
    QString getSensorType() const;
    QString getProfileId() const;
    QString getProfileName() const;
    QString getComponentType() const;

    // Scenarioconfig methods
    bool isScenarioconfigEnabled() const;
    QString getScType() const;
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

    // ✅ OLD METHOD NAMES FOR BACKWARD COMPATIBILITY
    bool isScEnabled() const { return isScenarioconfigEnabled(); }
    int getRange() const { return getScRange(); }

private slots:
    void onScCheckBoxStateChanged(int state);

private:
    void setupUI(DialogType type);
    void setupScSection();
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
    void populateProfiles(const QString& profileType);

    QLineEdit *nameLineEdit;
    QLineEdit *numberLineEdit;
    QComboBox *sensorTypeComboBox;
    QComboBox *profileComboBox;

    // Scenarioconfig widgets
    QCheckBox *scCheckBox;
    QGroupBox *scOptionsGroup;
    QComboBox *scTypeComboBox;
    QLineEdit *rangeLineEdit;
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
};

#endif // ADDITEMDIALOG_H

/* =============================================================================
 * FILE:         additemdialog.h
 * MODULE:       Add Item Dialog
 * PROJECT:      Indigenous Scenario and Sensor Simulation Toolkit (ISSST)
 * ORGANISATION: Oxygen 2 Innovation (O2I).
 * STANDARD:     RTCA DO-178C / ED-12C, DAL B
 * COVERAGE:     Branch / Decision Coverage required (100% true/false paths)
 *
 * DESCRIPTION:  Declares the AddItemDialog class which provides a modal dialog
 *               for adding new items (entities or folders) with configurable
 *               properties and components. Supports entity/folder creation,
 *               sensor/IFF/radio/weapon component configuration, scenario
 *               parameters (range, speed, turn radius, trajectory), profile
 *               selection, and entity component inheritance. The dialog adapts
 *               its UI based on dialog type (Entity/Folder) and mode (Normal,
 *               ComponentSensor, ComponentIFF, ComponentRadio, ComponentWeapon).
 *
 * REQUIREMENTS: REQ-DIALOG-010  Add entity or folder dialog
 *               REQ-DIALOG-011  Configure entity name and number/ID
 *               REQ-DIALOG-012  Select component types (sensor, IFF, radio, weapon)
 *               REQ-DIALOG-013  Configure scenario parameters (range, speed, etc.)
 *               REQ-DIALOG-014  Select trajectory type and parameters
 *               REQ-DIALOG-015  Populate profiles from hierarchy (sensor, IFF, etc.)
 *               REQ-DIALOG-016  Inherit components from existing entity
 *               REQ-DIALOG-017  Validate user inputs before acceptance
 *               REQ-DIALOG-018  Return configured component map
 *
 * AUTHOR:       Arti Rajpoot
 * REVIEWED BY:  [Reviewer Name], [Review Date] — SPR-DIALOG-001
 *
 *
 * COPYRIGHT:    Oxygen 2 Innovation (O2I). All rights reserved.
 *               Restricted circulation — defence simulation use only.
 * =============================================================================
 */

#ifndef ADDITEMDIALOG_H
#define ADDITEMDIALOG_H

#include "GUI/Hierarchytree/customtrajectorydialog.h"
#include <QDialog>
#include <QVariantMap>
#include <QHBoxLayout>
#include <QCompleter>
#include <QStringListModel>
#include <QCompleter>
#include <QStringListModel>
class QLineEdit;
class QComboBox;
class QCheckBox;
class QGroupBox;
class QSpinBox;
class Hierarchy;

// %%% Class Definition %%%
/* Dialog window for adding entities/folders with configurable properties */
class AddItemDialog : public QDialog
{
    Q_OBJECT

public:
    // %%% Dialog Type Enumeration %%%
    /* Defines the type of item being added */


    QString getSelectedEntityId() const;
    enum DialogType {
        EntityType,
        FolderType
    };

    // %%% Dialog Mode Enumeration %%%
    /* Defines specialized modes for component-specific dialogs */
    enum DialogMode {
        NormalMode,
        ComponentSensorMode,
        ComponentIFFMode,
        ComponentRadioMode,
        ComponentWeaponMode
    };

    QVariantMap getEntityComponents() const;
    // %%% Constructor %%%
    /* Initializes dialog with specified type and mode */
    AddItemDialog(DialogType type,
                  const QString &specificType,
                  DialogMode dialogMode,
                  Hierarchy* hierarchy,
                  QWidget *parent = nullptr,
                  const QString& editorContext = "");

    CustomTrajectoryDialog customDialog;
    // %%% Basic Property Accessors %%%
    /* Get the name entered in dialog */
    QString getName() const;
    /* Get the number/ID entered in dialog */
    void setNumber(int value);
    int getNumber() const;
    /* Get map of selected components and their states */
    QVariantMap getComponents() const;
    /* Get selected sensor type */
    QString getSensorType() const;
    /* Get selected profile ID */
    QString getProfileId() const;
    /* Get selected profile name */
    QString getProfileName() const;
    /* Get type of component being configured */
    QString getComponentType() const;

    // %%% Scenario Configuration Accessors %%%
    /* Check if scenario configuration is enabled */
    bool isScenarioconfigEnabled() const;
    /* Get scenario type selection */
    QString getScType() const;

    QPointF getCity() const;
    /* Get scenario range value */
    int getScRange() const;
    /* Get minimum radio range */
    int getMinRadioRange() const;
    /* Get maximum radio range */
    int getMaxRadioRange() const;
    /* Get minimum radar range */
    int getMinRadarRange() const;
    /* Get maximum radar range */
    int getMaxRadarRange() const;
    /* Get trajectory type */
    QString getTrajectory() const;
    /* Get minimum plane speed */
    int getMinPlaneSpeed() const;
    /* Get maximum plane speed */
    int getMaxPlaneSpeed() const;
    /* Get minimum turn radius */
    int getMinTurnRadius() const;
    /* Get maximum turn radius */
    int getMaxTurnRadius() const;

    // %%% Backward Compatibility Methods %%%
    /* Legacy method name for scenario config enabled state */
    bool isScEnabled() const { return isScenarioconfigEnabled(); }
    /* Legacy method name for scenario range */
    int getRange() const { return getScRange(); }
    // void populateEntityProfiles(const QString &profileTypeFilter = "");
    void populateEntityProfiles(const QString &profileTypeFilter = "",
                                const QString &categoryFilter = "All");
    QString getSelectedTeam() const;
    QString getNewComponentType() const;
    bool isNewSensorRequested() const { return m_createNewSensor; }


private slots:
    // %%% Private Slots %%%
    /* Handle scenario checkbox state changes */
    void onScCheckBoxStateChanged(int state);
protected:
    bool eventFilter(QObject *watched, QEvent *event) override;
private:
    // %%% UI Setup Methods %%%
    /* Set up user interface based on dialog type */
    QString m_editorContext;
    QString selectedEntityId;  // Store selected entity ID
    QVariantMap entityComponents;  // Store components from selected entity
    void setupUI(DialogType type);
    /* Set up scenario configuration section */
    void setupScSection();
    bool detectDatabaseEditorFromWindow();
    bool validateInputs();
    /* Create min-max spinbox pair with labels and units */
    QHBoxLayout* createMinMaxSpinBoxPair(const QString& label,
                                         QSpinBox*& minSpinBox,
                                         QSpinBox*& maxSpinBox,
                                         int minDefault,
                                         int maxDefault,
                                         int minRange,
                                         int maxRange,
                                         QString unit = " m");

    // %%% Profile Population Methods %%%
    /* Populate sensor profiles in combo box */
    void populateSensorProfiles();
    /* Populate IFF profiles in combo box */
    void populateIFFProfiles();
    /* Populate radio profiles in combo box */
    void populateRadioProfiles();
    /* Populate weapon profiles in combo box (mirrors Radio pattern) */
    void populateWeaponProfiles();
    /* General method to populate profiles by type */
    void populateProfiles(const QString& profileType);

    // %%% UI Widget Members %%%
    QLineEdit *nameLineEdit;            // Input for item name
    QLineEdit *numberLineEdit;          // Input for item number/ID
    QComboBox *sensorTypeComboBox;      // Dropdown for sensor type selection
    QComboBox *profileComboBox;         // Dropdown for profile selection
    QComboBox *entityComboBox;
    // %%% Scenario Configuration Widgets %%%
    QCheckBox *scCheckBox;              // Checkbox to enable scenario config
    QGroupBox *scOptionsGroup;          // Group box for scenario options
    QComboBox *scTypeComboBox;          // Dropdown for scenario type
    QComboBox *cityComboBox;
    QLineEdit *rangeLineEdit;           // Input for range value
    QSpinBox *minRadioRangeSpinBox;     // Spinbox for minimum radio range
    QSpinBox *maxRadioRangeSpinBox;     // Spinbox for maximum radio range
    QSpinBox *minRadarRangeSpinBox;     // Spinbox for minimum radar range
    QSpinBox *maxRadarRangeSpinBox;     // Spinbox for maximum radar range
    QComboBox *trajectoryComboBox;      // Dropdown for trajectory type
    QSpinBox *minPlaneSpeedSpinBox;     // Spinbox for minimum plane speed
    QSpinBox *maxPlaneSpeedSpinBox;     // Spinbox for maximum plane speed
    QSpinBox *minTurnRadiusSpinBox;     // Spinbox for minimum turn radius
    QSpinBox *maxTurnRadiusSpinBox;     // Spinbox for maximum turn radius

    // %%% Component Management %%%
    QMap<QString, QCheckBox*> componentCheckboxes;

    // %%% Configuration Members %%%
    QString specificType;    // Specific type of item being created
    DialogMode m_dialogMode; // Current dialog mode
    Hierarchy* m_hierarchy;  // Reference to hierarchy for profile data
    void onEntitySelected(const QString& entityId, const QString& profileName);
    void populateComponentsFromEntity(const QString& entityId, const QString& profileName);
    Hierarchy* m_tempDialogHierarchy;
    Hierarchy* m_tempHierarchyForDialog;
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
    QComboBox* m_newTypeCombo;
    bool m_createNewSensor = false;
    bool        m_addNewMode            = false;
    QWidget*    m_entitySearchContainer = nullptr;
    QPushButton* m_addNewBtn            = nullptr;
    QWidget* m_sensorTypeContainer = nullptr;


};

#endif // ADDITEMDIALOG_H

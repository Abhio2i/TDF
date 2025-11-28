
#ifndef ENTITYINFODIALOG_H
#define ENTITYINFODIALOG_H

// #include "GUI/Tacticaldisplay/canvaswidget.h"
#include "GUI/Tacticaldisplay/canvaswidget.h"
#include <QDialog>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QLabel>
#include <QScrollArea>
#include <QCheckBox>
#include <QPushButton>
#include <QVariantMap>
#include <QTableWidget>

class CanvasWidget;
class EntityInfoDialog : public QDialog
{
    Q_OBJECT

public:
    explicit EntityInfoDialog(QWidget *parent = nullptr);
    void setEntityInfo(const QString& entityId, MeshEntry *info);
    void updateEntityInfo();
    void clearInfo();
signals:
    void update();
private slots:
    void onCloseClicked();
    void onWeaponsClicked();
    void onSensorsClicked();
    void onFormationClicked();
    void onRadiosClicked();
    void onIFFClicked();

private:
    void setupUI();
    void createAttributeSection();
    void createCarrierSection();
    void createPositionSection();
    void createSpeedAltTableSection();
    void createTrackSection();
    void createActiveSection();
    void createEquipmentSection();
    void createOptionsSection();

    MeshEntry* entryInfo;
    // Main widgets
    QVBoxLayout *mainLayout;
    QLabel *titleLabel;
    QScrollArea *scrollArea;
    QWidget *scrollWidget;
    QVBoxLayout *scrollLayout;
    QPushButton *closeButton;

    // Table widgets
    QTableWidget *attributeTable;
    QTableWidget *speedAltTable;

    // Other widgets
    QLabel *carrierLabel;

    // Position section widgets
    QLabel *positionLabel;

    // Track section
    QVBoxLayout *trackLayout;
    QCheckBox *trackCheckBox;
    QCheckBox *centreCheckBox;
    QCheckBox *aggregatedScriptCheckBox;

    // Active section
    QCheckBox *activeCheckBox;

    // Equipment section
    QGridLayout *equipmentLayout;
    QPushButton *weaponsButton;
    QPushButton *sensorsButton;
    QPushButton *formationButton;
    QPushButton *radiosButton;
    QPushButton *iffButton;

    // Options section
    QVBoxLayout *optionsLayout;
    QCheckBox *followTrajectoryCheckBox;
    QCheckBox *showConnectionCheckBox;
    QCheckBox *showDetectionCheckBox;
    QCheckBox *controlDecisiveCheckBox;

    // Data
    QString currentEntityId;
    QVariantMap currentEntityData;


    // 🆕 Helper function for real-time sensors updates
    void updateSensorsTable(QTableWidget* sensorsTable, Entity* entity);

};

#endif // ENTITYINFODIALOG_H

// // // // #ifndef ENTITYINFODIALOG_H
// // // // #define ENTITYINFODIALOG_H

// // // // #include <QDialog>
// // // // #include <QVBoxLayout>
// // // // #include <QLabel>
// // // // #include <QTextEdit>
// // // // #include <QPushButton>
// // // // #include <QTimer>

// // // // class EntityInfoDialog : public QDialog
// // // // {
// // // //     Q_OBJECT

// // // // public:
// // // //     explicit EntityInfoDialog(QWidget *parent = nullptr);
// // // //     void setEntityInfo(const QString& entityId, const QVariantMap& entityData);
// // // //     void clearInfo();

// // // // private slots:
// // // //     void onCloseClicked();

// // // // private:
// // // //     QLabel *titleLabel;
// // // //     QTextEdit *infoTextEdit;
// // // //     QPushButton *closeButton;
// // // //     QVBoxLayout *mainLayout;
// // // // };

// // // // #endif // ENTITYINFODIALOG_H




// // // #ifndef ENTITYINFODIALOG_H
// // // #define ENTITYINFODIALOG_H

// // // #include <QDialog>
// // // #include <QVBoxLayout>
// // // #include <QHBoxLayout>
// // // #include <QGridLayout>
// // // #include <QLabel>
// // // #include <QTextEdit>
// // // #include <QPushButton>
// // // #include <QCheckBox>
// // // #include <QGroupBox>
// // // #include <QScrollArea>
// // // #include <QFrame>
// // // #include <QTimer>
// // // #include <QVariantMap>

// // // class EntityInfoDialog : public QDialog
// // // {
// // //     Q_OBJECT

// // // public:
// // //     explicit EntityInfoDialog(QWidget *parent = nullptr);
// // //     void setEntityInfo(const QString& entityId, const QVariantMap& entityData);
// // //     void clearInfo();

// // // private slots:
// // //     void onCloseClicked();
// // //     void onWeaponsClicked();
// // //     void onSensorsClicked();
// // //     void onFormationClicked();
// // //     void onRadiosClicked();
// // //     void onIFFClicked();

// // // private:
// // //     void setupUI();
// // //     void createAttributeSection();
// // //     void createCarrierSection();
// // //     void createPositionSection();
// // //     void createTrackSection();
// // //     void createActiveSection();
// // //     void createEquipmentSection();
// // //     void createOptionsSection();

// // //     // Main widgets
// // //     QLabel *titleLabel;
// // //     QScrollArea *scrollArea;
// // //     QWidget *scrollWidget;
// // //     QVBoxLayout *mainLayout;
// // //     QVBoxLayout *scrollLayout;
// // //     QPushButton *closeButton;

// // //     // Section widgets
// // //     QGroupBox *attributeGroup;
// // //     QGroupBox *carrierGroup;
// // //     QGroupBox *positionGroup;
// // //     QGroupBox *trackGroup;
// // //     QGroupBox *activeGroup;
// // //     QGroupBox *equipmentGroup;
// // //     QGroupBox *optionsGroup;

// // //     // Attribute section
// // //     QLabel *typeLabel;
// // //     QLabel *nameLabel;
// // //     QLabel *disnameLabel;

// // //     // Carrier section
// // //     QLabel *carrierLabel;

// // //     // Position section
// // //     QLabel *attributeCurrentLabel;
// // //     QLabel *attributeRequestedLabel;
// // //     QLabel *altLabel;
// // //     QLabel *speedLabel;

// // //     // Track section
// // //     QCheckBox *trackCheckBox;
// // //     QCheckBox *centreCheckBox;
// // //     QCheckBox *aggregatedScriptCheckBox;

// // //     // Active section
// // //     QCheckBox *activeCheckBox;

// // //     // Equipment section
// // //     QPushButton *weaponsButton;
// // //     QPushButton *sensorsButton;
// // //     QPushButton *formationButton;
// // //     QPushButton *radiosButton;
// // //     QPushButton *iffButton;

// // //     // Options section
// // //     QCheckBox *followTrajectoryCheckBox;
// // //     QCheckBox *showConnectionCheckBox;
// // //     QCheckBox *showDetectionCheckBox;
// // //     QCheckBox *controlDecisiveCheckBox;

// // //     QString currentEntityId;
// // //     QVariantMap currentEntityData;
// // // };

// // // #endif // ENTITYINFODIALOG_H
// // #ifndef ENTITYINFODIALOG_H
// // #define ENTITYINFODIALOG_H

// // #include <QDialog>
// // #include <QVBoxLayout>
// // #include <QHBoxLayout>
// // #include <QGridLayout>
// // #include <QLabel>
// // #include <QScrollArea>
// // #include <QGroupBox>
// // #include <QCheckBox>
// // #include <QPushButton>
// // #include <QVariantMap>
// // #include <QTableWidget>

// // class EntityInfoDialog : public QDialog
// // {
// //     Q_OBJECT

// // public:
// //     explicit EntityInfoDialog(QWidget *parent = nullptr);
// //     void setEntityInfo(const QString& entityId, const QVariantMap& entityData);
// //     void clearInfo();

// // private slots:
// //     void onCloseClicked();
// //     void onWeaponsClicked();
// //     void onSensorsClicked();
// //     void onFormationClicked();
// //     void onRadiosClicked();
// //     void onIFFClicked();

// // private:
// //     void setupUI();
// //     void createAttributeSection();
// //     void createCarrierSection();
// //     void createPositionSection();
// //     void createTrackSection();
// //     void createActiveSection();
// //     void createEquipmentSection();
// //     void createOptionsSection();

// //     // Main widgets
// //     QVBoxLayout *mainLayout;
// //     QLabel *titleLabel;
// //     QScrollArea *scrollArea;
// //     QWidget *scrollWidget;
// //     QVBoxLayout *scrollLayout;
// //     QPushButton *closeButton;

// //     // Section groups
// //     QGroupBox *attributeGroup;
// //     QGroupBox *carrierGroup;
// //     QGroupBox *positionGroup;
// //     QGroupBox *trackGroup;
// //     QGroupBox *activeGroup;
// //     QGroupBox *equipmentGroup;
// //     QGroupBox *optionsGroup;

// //     // Table widgets
// //     QTableWidget *attributeTable;
// //     QTableWidget *positionTable;

// //     // Other widgets
// //     QLabel *carrierLabel;
// //     QCheckBox *trackCheckBox;
// //     QCheckBox *centreCheckBox;
// //     QCheckBox *aggregatedScriptCheckBox;
// //     QCheckBox *activeCheckBox;
// //     QCheckBox *followTrajectoryCheckBox;
// //     QCheckBox *showConnectionCheckBox;
// //     QCheckBox *showDetectionCheckBox;
// //     QCheckBox *controlDecisiveCheckBox;
// //     QPushButton *weaponsButton;
// //     QPushButton *sensorsButton;
// //     QPushButton *formationButton;
// //     QPushButton *radiosButton;
// //     QPushButton *iffButton;

// //     // Data
// //     QString currentEntityId;
// //     QVariantMap currentEntityData;
// // };

// // #endif // ENTITYINFODIALOG_H


// #ifndef ENTITYINFODIALOG_H
// #define ENTITYINFODIALOG_H

// #include <QDialog>
// #include <QVBoxLayout>
// #include <QHBoxLayout>
// #include <QGridLayout>
// #include <QLabel>
// #include <QScrollArea>
// #include <QCheckBox>
// #include <QPushButton>
// #include <QVariantMap>
// #include <QTableWidget>

// class EntityInfoDialog : public QDialog
// {
//     Q_OBJECT

// public:
//     explicit EntityInfoDialog(QWidget *parent = nullptr);
//     void setEntityInfo(const QString& entityId, const QVariantMap& entityData);
//     void clearInfo();

// private slots:
//     void onCloseClicked();
//     void onWeaponsClicked();
//     void onSensorsClicked();
//     void onFormationClicked();
//     void onRadiosClicked();
//     void onIFFClicked();

// private:
//     void setupUI();
//     void createAttributeSection();
//     void createCarrierSection();
//     void createPositionSection();
//     void createTrackSection();
//     void createActiveSection();
//     void createEquipmentSection();
//     void createOptionsSection();

//     // Main widgets
//     QVBoxLayout *mainLayout;
//     QLabel *titleLabel;
//     QScrollArea *scrollArea;
//     QWidget *scrollWidget;
//     QVBoxLayout *scrollLayout;
//     QPushButton *closeButton;

//     // Table widget for attributes
//     QTableWidget *attributeTable;

//     // Other widgets
//     QLabel *carrierLabel;

//     // Position section widgets
//     QVBoxLayout *positionLayout;
//     QLabel *positionCurrentLabel;
//     QLabel *positionRequestedLabel;
//     QLabel *positionAltLabel;
//     QLabel *positionSpeedLabel;

//     // Track section
//     QVBoxLayout *trackLayout;
//     QCheckBox *trackCheckBox;
//     QCheckBox *centreCheckBox;
//     QCheckBox *aggregatedScriptCheckBox;

//     // Active section
//     QCheckBox *activeCheckBox;

//     // Equipment section
//     QGridLayout *equipmentLayout;
//     QPushButton *weaponsButton;
//     QPushButton *sensorsButton;
//     QPushButton *formationButton;
//     QPushButton *radiosButton;
//     QPushButton *iffButton;

//     // Options section
//     QVBoxLayout *optionsLayout;
//     QCheckBox *followTrajectoryCheckBox;
//     QCheckBox *showConnectionCheckBox;
//     QCheckBox *showDetectionCheckBox;
//     QCheckBox *controlDecisiveCheckBox;

//     // Data
//     QString currentEntityId;
//     QVariantMap currentEntityData;
// };

// #endif // ENTITYINFODIALOG_H
#ifndef ENTITYINFODIALOG_H
#define ENTITYINFODIALOG_H

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

class EntityInfoDialog : public QDialog
{
    Q_OBJECT

public:
    explicit EntityInfoDialog(QWidget *parent = nullptr);
    void setEntityInfo(const QString& entityId, const QVariantMap& entityData);
    void clearInfo();

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
    QVBoxLayout *positionLayout;
    QLabel *positionCurrentLabel;
    QLabel *positionRequestedLabel;

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
};

#endif // ENTITYINFODIALOG_H

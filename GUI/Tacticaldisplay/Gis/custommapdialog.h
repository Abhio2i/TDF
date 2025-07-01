// // #ifndef CUSTOMMAPDIALOG_H
// // #define CUSTOMMAPDIALOG_H

// // #include <QDialog>
// // #include <QLineEdit>
// // #include <QSpinBox>
// // #include <QVBoxLayout>
// // #include <QFormLayout>
// // #include <QDialogButtonBox>

// // class CustomMapDialog : public QDialog {
// //     Q_OBJECT
// // public:
// //     explicit CustomMapDialog(QWidget *parent = nullptr);
// //     int getZoomMin() const;
// //     int getZoomMax() const;
// //     QString getTileUrl() const;
// //     QString getMapName() const;

// // private:
// //     QSpinBox *zoomMinSpinBox;
// //     QSpinBox *zoomMaxSpinBox;
// //     QLineEdit *tileUrlEdit;
// //     QLineEdit *mapNameEdit;
// // };

// // #endif // CUSTOMMAPDIALOG_H


// #ifndef CUSTOMMAPDIALOG_H
// #define CUSTOMMAPDIALOG_H

// #include <QDialog>
// #include <QLineEdit>
// #include <QSpinBox>
// #include <QVBoxLayout>
// #include <QFormLayout>
// #include <QDialogButtonBox>
// #include <QDoubleSpinBox>
// #include <QComboBox>

// class CustomMapDialog : public QDialog {
//     Q_OBJECT
// public:
//     explicit CustomMapDialog(QWidget *parent = nullptr);
//     CustomMapDialog(const QString &name, const QString &tileUrl, int zoomMin, int zoomMax,
//                     qreal opacity, const QString &resolution, const QString &type, QWidget *parent = nullptr);

//     QString getMapName() const;
//     QString getTileUrl() const;
//     int getZoomMin() const;
//     int getZoomMax() const;
//     qreal getOpacity() const;
//     QString getResolution() const;
//     QString getType() const;

// private:
//     void setupUi();
//     QLineEdit *mapNameEdit;
//     QLineEdit *tileUrlEdit;
//     QSpinBox *zoomMinSpinBox;
//     QSpinBox *zoomMaxSpinBox;
//     QDoubleSpinBox *opacitySpinBox;
//     QLineEdit *resolutionEdit;
//     QComboBox *typeComboBox;


// };

// #endif // CUSTOMMAPDIALOG_H




#ifndef CUSTOMMAPDIALOG_H
#define CUSTOMMAPDIALOG_H

#include <QDialog>
#include <QLineEdit>
#include <QSpinBox>
#include <QVBoxLayout>
#include <QFormLayout>
#include <QDialogButtonBox>
#include <QComboBox>

class CustomMapDialog : public QDialog {
    Q_OBJECT
public:
    explicit CustomMapDialog(QWidget *parent = nullptr);
    CustomMapDialog(const QString &name, const QString &tileUrl, int zoomMin, int zoomMax,
                    qreal opacity, const QString &resolution, const QString &type, QWidget *parent = nullptr);

    QString getMapName() const;
    QString getTileUrl() const;
    int getZoomMin() const;
    int getZoomMax() const;
    qreal getOpacity() const;
    QString getResolution() const;
    QString getType() const;

private:
    void setupUi();
    QLineEdit *mapNameEdit;
    QLineEdit *tileUrlEdit;
    QSpinBox *zoomMinSpinBox;
    QSpinBox *zoomMaxSpinBox;
    QComboBox *opacityComboBox; // Changed to QComboBox
    QComboBox *resolutionComboBox; // Changed to QComboBox
    QComboBox *typeComboBox;
};

#endif // CUSTOMMAPDIALOG_H

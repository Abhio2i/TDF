
// #ifndef CUSTOMMAPDIALOG_H
// #define CUSTOMMAPDIALOG_H

// #include <QDialog>
// #include <QLineEdit>
// #include <QSpinBox>
// #include <QVBoxLayout>
// #include <QFormLayout>
// #include <QDialogButtonBox>
// #include <QComboBox>

// class CustomMapDialog : public QDialog {
//     Q_OBJECT
// public:
//     explicit CustomMapDialog(QWidget *parent = nullptr);
//     CustomMapDialog(const QString &name, const QString &tileUrl, int zoomMin, int zoomMax,
//                     qreal opacity, const QString &type, QWidget *parent = nullptr);

//     QString getMapName() const;
//     QString getTileUrl() const;
//     int getZoomMin() const;
//     int getZoomMax() const;
//     qreal getOpacity() const;
//     // QString getResolution() const;
//     QString getType() const;

// private:
//     void setupUi();
//     QLineEdit *mapNameEdit;
//     QLineEdit *tileUrlEdit;
//     QSpinBox *zoomMinSpinBox;
//     QSpinBox *zoomMaxSpinBox;
//     QComboBox *opacityComboBox;
//     QComboBox *typeComboBox;
// };

// #endif // CUSTOMMAPDIALOG_H

/*
 * CustomMapDialog Header File
 * This file defines the CustomMapDialog class which provides a user interface
 * for creating and editing custom map layer configurations with various parameters
 * including tile URLs, zoom ranges, opacity, and layer types.
 */

#ifndef CUSTOMMAPDIALOG_H
#define CUSTOMMAPDIALOG_H

// Include necessary Qt classes
#include <QDialog>
#include <QLineEdit>
#include <QSpinBox>
#include <QVBoxLayout>
#include <QFormLayout>
#include <QDialogButtonBox>
#include <QComboBox>

/*
 * CustomMapDialog Class
 * A dialog window that allows users to configure custom map layers
 * with parameters like name, tile URL, zoom range, opacity, and layer type.
 */
class CustomMapDialog : public QDialog {
    Q_OBJECT  // Qt macro for signals/slots support

public:
    /*
     * Constructor: Create a new custom map dialog
     * Initializes an empty dialog for creating new custom map layers
     */
    explicit CustomMapDialog(QWidget *parent = nullptr);

    /*
     * Constructor: Create a custom map dialog with pre-filled values
     * Used for editing existing custom map layer configurations
     */
    CustomMapDialog(const QString &name, const QString &tileUrl, int zoomMin, int zoomMax,
                    qreal opacity, const QString &type, QWidget *parent = nullptr);

    /*
     * Accessor Methods for Dialog Data
     * These methods retrieve the configured values from the dialog inputs
     */

    /* Get the user-defined map layer name */
    QString getMapName() const;

    /* Get the tile server URL template */
    QString getTileUrl() const;

    /* Get the minimum zoom level for the layer */
    int getZoomMin() const;

    /* Get the maximum zoom level for the layer */
    int getZoomMax() const;

    /* Get the opacity value for the layer (0.0 to 1.0) */
    qreal getOpacity() const;

    /* Get the selected layer type (e.g., "Raster", "Vector", etc.) */
    QString getType() const;

private:
    /*
     * Private Setup Method
     * Initializes the dialog UI components and layout
     */
    void setupUi();

    /*
     * UI Component Members
     * These widgets capture user input for custom map configuration
     */

    QLineEdit *mapNameEdit;        // Input for custom map layer name
    QLineEdit *tileUrlEdit;        // Input for tile server URL template
    QSpinBox *zoomMinSpinBox;      // Spin box for minimum zoom level
    QSpinBox *zoomMaxSpinBox;      // Spin box for maximum zoom level
    QComboBox *opacityComboBox;    // Dropdown for opacity selection
    QComboBox *typeComboBox;       // Dropdown for layer type selection
};

#endif // CUSTOMMAPDIALOG_H

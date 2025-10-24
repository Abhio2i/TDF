// #include "custommapdialog.h"
// CustomMapDialog::CustomMapDialog(QWidget *parent) : QDialog(parent) {
//     setWindowTitle("Add Custom Map");
//     setupUi();
// }
// CustomMapDialog::CustomMapDialog(const QString &name, const QString &tileUrl, int zoomMin, int zoomMax,
//                                  qreal opacity, const QString &type, QWidget *parent)
//     : QDialog(parent) {
//     setWindowTitle("Edit Custom Map");
//     setupUi();
//     mapNameEdit->setText(name);
//     tileUrlEdit->setText(tileUrl);
//     zoomMinSpinBox->setValue(zoomMin);
//     zoomMaxSpinBox->setValue(zoomMax);
//     QString opacityText = opacity >= 0.0 ? QString::number(opacity * 100, 'f', 0) + "%" : "100%";
//     opacityComboBox->setCurrentText(opacityText);
//     typeComboBox->setCurrentText(type.isEmpty() || type == "N/A" ? "Raster" : type);
// }

// void CustomMapDialog::setupUi() {
//     QVBoxLayout *layout = new QVBoxLayout(this);
//     QFormLayout *formLayout = new QFormLayout();

//     mapNameEdit = new QLineEdit(this);
//     mapNameEdit->setPlaceholderText("e.g., My Custom Map");
//     formLayout->addRow("Map Name:", mapNameEdit);

//     tileUrlEdit = new QLineEdit(this);
//     tileUrlEdit->setPlaceholderText("e.g., https://example.com/{z}/{x}/{y}.png");
//     formLayout->addRow("Map URL:", tileUrlEdit);

//     zoomMinSpinBox = new QSpinBox(this);
//     zoomMinSpinBox->setRange(0, 30);
//     zoomMinSpinBox->setValue(0);
//     formLayout->addRow("Minimum Zoom Level:", zoomMinSpinBox);

//     zoomMaxSpinBox = new QSpinBox(this);
//     zoomMaxSpinBox->setRange(0, 30);
//     zoomMaxSpinBox->setValue(19);
//     formLayout->addRow("Maximum Zoom Level:", zoomMaxSpinBox);

//     opacityComboBox = new QComboBox(this);
//     opacityComboBox->addItems({"0%", "20%", "50%", "80%", "100%"});
//     opacityComboBox->setCurrentText("100%");
//     formLayout->addRow("Opacity:", opacityComboBox);


//     typeComboBox = new QComboBox(this);
//     typeComboBox->addItems({"Raster", "Vector", "Other"});
//     formLayout->addRow("Type:", typeComboBox);

//     layout->addLayout(formLayout);

//     QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
//     connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
//     connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
//     layout->addWidget(buttonBox);

//     setLayout(layout);
// }

// QString CustomMapDialog::getMapName() const {
//     return mapNameEdit->text().trimmed();
// }

// QString CustomMapDialog::getTileUrl() const {
//     return tileUrlEdit->text().trimmed();
// }

// int CustomMapDialog::getZoomMin() const {
//     return zoomMinSpinBox->value();
// }

// int CustomMapDialog::getZoomMax() const {
//     return zoomMaxSpinBox->value();
// }



// qreal CustomMapDialog::getOpacity() const {
//     QString selectedOpacity = opacityComboBox->currentText();
//     // Remove '%' and convert to qreal (0.0 to 1.0)
//     bool ok;
//     qreal opacity = selectedOpacity.remove('%').toDouble(&ok) / 100.0;
//     return ok ? opacity : 1.0; // Default to 1.0 if conversion fails
// }

// QString CustomMapDialog::getType() const {
//     return typeComboBox->currentText();
// }


/*
 * CustomMapDialog Implementation File
 * This file contains the implementation of the CustomMapDialog class which provides
 * a user interface for configuring custom map layers with various parameters including
 * name, tile URL, zoom range, opacity, and layer type.
 */

#include "custommapdialog.h"

/*
 * Constructor: CustomMapDialog (Default)
 * Creates a new custom map configuration dialog with default values
 * Used for adding new custom map layers to the application
 */
CustomMapDialog::CustomMapDialog(QWidget *parent) : QDialog(parent) {
    setWindowTitle("Add Custom Map");  // Set dialog title for add operation
    setupUi();  // Initialize the user interface components
}

/*
 * Constructor: CustomMapDialog (With Parameters)
 * Creates a custom map configuration dialog with pre-filled values
 * Used for editing existing custom map layer configurations
 */
CustomMapDialog::CustomMapDialog(const QString &name, const QString &tileUrl, int zoomMin, int zoomMax,
                                 qreal opacity, const QString &type, QWidget *parent)
    : QDialog(parent) {
    setWindowTitle("Edit Custom Map");  // Set dialog title for edit operation
    setupUi();  // Initialize the user interface components

    // Pre-fill form fields with existing values
    mapNameEdit->setText(name);                    // Set map name
    tileUrlEdit->setText(tileUrl);                 // Set tile URL template
    zoomMinSpinBox->setValue(zoomMin);             // Set minimum zoom level
    zoomMaxSpinBox->setValue(zoomMax);             // Set maximum zoom level

    // Convert opacity value to percentage string for display
    QString opacityText = opacity >= 0.0 ? QString::number(opacity * 100, 'f', 0) + "%" : "100%";
    opacityComboBox->setCurrentText(opacityText);  // Set opacity selection

    // Set layer type, default to "Raster" if not specified
    typeComboBox->setCurrentText(type.isEmpty() || type == "N/A" ? "Raster" : type);
}

/*
 * setupUi: Initialize the dialog user interface
 * Creates and arranges all form elements and controls in the dialog
 */
void CustomMapDialog::setupUi() {
    // Create main vertical layout for the dialog
    QVBoxLayout *layout = new QVBoxLayout(this);

    // Create form layout for organized input fields
    QFormLayout *formLayout = new QFormLayout();

    /*
     * Map Name Input Field
     * Text field for entering a descriptive name for the custom map layer
     */
    mapNameEdit = new QLineEdit(this);
    mapNameEdit->setPlaceholderText("e.g., My Custom Map");  // Hint text for user
    formLayout->addRow("Map Name:", mapNameEdit);            // Add to form with label

    /*
     * Tile URL Input Field
     * Text field for entering the tile server URL template
     * Supports placeholders like {z}, {x}, {y} for dynamic tile coordinates
     */
    tileUrlEdit = new QLineEdit(this);
    tileUrlEdit->setPlaceholderText("e.g., https://example.com/{z}/{x}/{y}.png");
    formLayout->addRow("Map URL:", tileUrlEdit);

    /*
     * Minimum Zoom Level Spin Box
     * Numeric input for the lowest zoom level at which this layer should be visible
     */
    zoomMinSpinBox = new QSpinBox(this);
    zoomMinSpinBox->setRange(0, 30);    // Allow zoom levels from 0 to 30
    zoomMinSpinBox->setValue(0);        // Default to minimum zoom
    formLayout->addRow("Minimum Zoom Level:", zoomMinSpinBox);

    /*
     * Maximum Zoom Level Spin Box
     * Numeric input for the highest zoom level at which this layer should be visible
     */
    zoomMaxSpinBox = new QSpinBox(this);
    zoomMaxSpinBox->setRange(0, 30);    // Allow zoom levels from 0 to 30
    zoomMaxSpinBox->setValue(19);       // Default to typical maximum zoom
    formLayout->addRow("Maximum Zoom Level:", zoomMaxSpinBox);

    /*
     * Opacity Selection Combo Box
     * Dropdown for selecting the transparency level of the map layer
     * Values range from completely transparent (0%) to fully opaque (100%)
     */
    opacityComboBox = new QComboBox(this);
    opacityComboBox->addItems({"0%", "20%", "50%", "80%", "100%"});  // Available opacity levels
    opacityComboBox->setCurrentText("100%");  // Default to fully opaque
    formLayout->addRow("Opacity:", opacityComboBox);

    /*
     * Layer Type Selection Combo Box
     * Dropdown for specifying the type of map layer
     * Helps categorize and potentially apply different rendering logic
     */
    typeComboBox = new QComboBox(this);
    typeComboBox->addItems({"Raster", "Vector", "Other"});  // Available layer types
    formLayout->addRow("Type:", typeComboBox);

    // Add the form layout to the main dialog layout
    layout->addLayout(formLayout);

    /*
     * Dialog Button Box
     * Standard OK and Cancel buttons for accepting or rejecting the dialog
     */
    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);  // Connect OK to accept
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);  // Connect Cancel to reject
    layout->addWidget(buttonBox);  // Add button box to layout

    // Set the main layout for the dialog
    setLayout(layout);
}

/*
 * getMapName: Retrieve the configured map name
 * Returns the user-entered name for the custom map layer
 */
QString CustomMapDialog::getMapName() const {
    return mapNameEdit->text().trimmed();  // Return trimmed text to remove whitespace
}

/*
 * getTileUrl: Retrieve the configured tile URL template
 * Returns the URL pattern for accessing map tiles with placeholders
 */
QString CustomMapDialog::getTileUrl() const {
    return tileUrlEdit->text().trimmed();  // Return trimmed URL template
}

/*
 * getZoomMin: Retrieve the minimum zoom level
 * Returns the lowest zoom level at which the layer should be displayed
 */
int CustomMapDialog::getZoomMin() const {
    return zoomMinSpinBox->value();  // Return current spin box value
}

/*
 * getZoomMax: Retrieve the maximum zoom level
 * Returns the highest zoom level at which the layer should be displayed
 */
int CustomMapDialog::getZoomMax() const {
    return zoomMaxSpinBox->value();  // Return current spin box value
}

/*
 * getOpacity: Retrieve the opacity value as a decimal
 * Converts the percentage string from combo box to a qreal value between 0.0 and 1.0
 */
qreal CustomMapDialog::getOpacity() const {
    QString selectedOpacity = opacityComboBox->currentText();  // Get selected opacity text

    // Remove '%' character and convert to numeric value
    bool ok;  // Success flag for conversion
    qreal opacity = selectedOpacity.remove('%').toDouble(&ok) / 100.0;  // Convert to decimal

    return ok ? opacity : 1.0; // Return converted value or 1.0 (fully opaque) if conversion fails
}

/*
 * getType: Retrieve the selected layer type
 * Returns the user-selected category for the map layer
 */
QString CustomMapDialog::getType() const {
    return typeComboBox->currentText();  // Return currently selected type
}

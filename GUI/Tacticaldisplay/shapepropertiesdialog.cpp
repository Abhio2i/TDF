//============================================================================
// ShapePropertiesDialog
// Purpose: Dialog for editing visual properties of a shape (border color and border thickness).
// Written by: Waris
//============================================================================

#include "shapepropertiesdialog.h"

// Constructor
// Initializes the dialog UI, applies a dark theme,
// and sets up color selection and border thickness controls.
ShapePropertiesDialog::ShapePropertiesDialog(QWidget *parent)
    : QDialog(parent), m_currentColor(Qt::red), m_borderThickness(2)
{
    setWindowTitle("Shape Properties");
    setFixedSize(300, 200);

    // Set dark theme
    setStyleSheet(
        "QDialog { background-color: #2b2b2b; color: white; }"
        "QLabel { color: white; font-weight: bold; }"
        "QSpinBox { background-color: #3c3c3c; color: white; border: 1px solid #555; padding: 5px; }"
        "QPushButton { background-color: #404040; color: white; border: 1px solid #555; padding: 8px; font-weight: bold; }"
        "QPushButton:hover { background-color: #505050; }"
        "QPushButton:pressed { background-color: #606060; }"
        );

    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    // Color selection
    QLabel *colorLabel = new QLabel("Shape Color:", this);
    m_colorButton = new QPushButton(this);
    m_colorButton->setStyleSheet(QString("background-color: %1; color: white; font-weight: bold; border: 2px solid white;").arg(m_currentColor.name()));
    m_colorButton->setText("Select Color");
    connect(m_colorButton, &QPushButton::clicked, this, &ShapePropertiesDialog::onColorButtonClicked);

    // Border thickness
    QLabel *thicknessLabel = new QLabel("Border Thickness:", this);
    m_thicknessSpinBox = new QSpinBox(this);
    m_thicknessSpinBox->setRange(1, 10);
    m_thicknessSpinBox->setValue(m_borderThickness);

    // Buttons
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    QPushButton *applyButton = new QPushButton("Apply", this);
    QPushButton *cancelButton = new QPushButton("Cancel", this);

    buttonLayout->addWidget(applyButton);
    buttonLayout->addWidget(cancelButton);

    // Add to main layout
    mainLayout->addWidget(colorLabel);
    mainLayout->addWidget(m_colorButton);
    mainLayout->addWidget(thicknessLabel);
    mainLayout->addWidget(m_thicknessSpinBox);
    mainLayout->addStretch();
    mainLayout->addLayout(buttonLayout);

    // Connect buttons
    connect(applyButton, &QPushButton::clicked, this, &QDialog::accept);
    connect(cancelButton, &QPushButton::clicked, this, &QDialog::reject);
}

// Updates the dialog UI with existing shape properties
void ShapePropertiesDialog::setCurrentProperties(const QColor& color, int borderThickness)
{
    m_currentColor = color;
    m_borderThickness = borderThickness;

    m_colorButton->setStyleSheet(QString("background-color: %1; color: white; font-weight: bold; border: 2px solid white;").arg(m_currentColor.name()));
    m_thicknessSpinBox->setValue(m_borderThickness);
}

// Returns the color selected by the user
QColor ShapePropertiesDialog::getSelectedColor() const
{
    return m_currentColor;
}

// Returns the selected border thickness
int ShapePropertiesDialog::getBorderThickness() const
{
    return m_thicknessSpinBox->value();
}

// Opens a color picker dialog and updates the preview button
void ShapePropertiesDialog::onColorButtonClicked()
{
    QColor newColor = QColorDialog::getColor(m_currentColor, this, "Select Shape Color", QColorDialog::DontUseNativeDialog);
    if (newColor.isValid()) {
        m_currentColor = newColor;
        m_colorButton->setStyleSheet(QString("background-color: %1; color: white; font-weight: bold; border: 2px solid white;").arg(m_currentColor.name()));
    }
}

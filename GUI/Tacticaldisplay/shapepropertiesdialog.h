#ifndef SHAPEPROPERTIESDIALOG_H
#define SHAPEPROPERTIESDIALOG_H

#include <QDialog>
#include <QColor>
#include <QSpinBox>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QColorDialog>

class ShapePropertiesDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ShapePropertiesDialog(QWidget *parent = nullptr);

    void setCurrentProperties(const QColor& color, int borderThickness);
    QColor getSelectedColor() const;
    int getBorderThickness() const;
     void setEntityInfo(const QString& entityId, const QVariantMap& entityData);

private slots:
    void onColorButtonClicked();

private:
    QColor m_currentColor;
    int m_borderThickness;

    QPushButton *m_colorButton;
    QSpinBox *m_thicknessSpinBox;
};

#endif // SHAPEPROPERTIESDIALOG_H

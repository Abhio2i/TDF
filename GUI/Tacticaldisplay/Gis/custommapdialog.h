#ifndef CUSTOMMAPDIALOG_H
#define CUSTOMMAPDIALOG_H

#include <QDialog>
#include <QLineEdit>
#include <QSpinBox>
#include <QVBoxLayout>
#include <QFormLayout>
#include <QDialogButtonBox>

class CustomMapDialog : public QDialog {
    Q_OBJECT
public:
    explicit CustomMapDialog(QWidget *parent = nullptr);
    int getZoomMin() const;
    int getZoomMax() const;
    QString getTileUrl() const;
    QString getMapName() const;

private:
    QSpinBox *zoomMinSpinBox;
    QSpinBox *zoomMaxSpinBox;
    QLineEdit *tileUrlEdit;
    QLineEdit *mapNameEdit;
};

#endif // CUSTOMMAPDIALOG_H

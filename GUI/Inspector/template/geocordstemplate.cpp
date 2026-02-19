#include "geocordstemplate.h"
#include <QHBoxLayout>
#include <QLineEdit>
#include <QDoubleValidator>
#include <QLabel>
#include "GUI/Inspector/inspector.h"
#include "GUI/Inspector/inspector-styles.h"

GeocordsTemplate::GeocordsTemplate(Inspector *inspector, QWidget *parent)
    : QWidget(parent), inspectorRef(inspector) {}

void GeocordsTemplate::setupGeocordsCell(int row, const QString &fullKey, const QJsonObject &obj, QTableWidget *tableWidget)
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(2, 2, 2, 2);
    mainLayout->setSpacing(2);

    QLineEdit *latEdit = new QLineEdit(this);
    QLineEdit *lonEdit = new QLineEdit(this);
    QLineEdit *altEdit = new QLineEdit(this);
    QLineEdit *headEdit = new QLineEdit(this);

    QDoubleValidator *validator = new QDoubleValidator(this);
    validator->setNotation(QDoubleValidator::StandardNotation);
    validator->setDecimals(6);

    latEdit->setValidator(validator);
    lonEdit->setValidator(validator);
    altEdit->setValidator(validator);
    headEdit->setValidator(validator);

    latEdit->setText(Inspector::formatNumberForUI(obj.value("latitude").toDouble(0.0)));
    lonEdit->setText(Inspector::formatNumberForUI(obj.value("longitude").toDouble(0.0)));
    altEdit->setText(Inspector::formatNumberForUI(obj.value("altitude").toDouble(0.0)));
    headEdit->setText(Inspector::formatNumberForUI(obj.value("heading").toDouble(0.0)));

    latEdit->setStyleSheet(InspectorStyles::GeocordsInput);
    lonEdit->setStyleSheet(InspectorStyles::GeocordsInput);
    altEdit->setStyleSheet(InspectorStyles::GeocordsInput);
    headEdit->setStyleSheet(InspectorStyles::GeocordsInput);

    QLabel *latLabel = new QLabel("Lat:", this);
    QLabel *lonLabel = new QLabel("Lon:", this);
    QLabel *altLabel = new QLabel("Alt:", this);
    QLabel *headLabel = new QLabel("Head:", this);

    latLabel->setStyleSheet(InspectorStyles::GeocordsLabel);
    lonLabel->setStyleSheet(InspectorStyles::GeocordsLabel);
    altLabel->setStyleSheet(InspectorStyles::GeocordsLabel);
    headLabel->setStyleSheet(InspectorStyles::GeocordsLabel);

    QHBoxLayout *latLayout = new QHBoxLayout();
    latLayout->addWidget(latLabel);
    latLayout->addWidget(latEdit);
    latLayout->addStretch();

    QHBoxLayout *lonLayout = new QHBoxLayout();
    lonLayout->addWidget(lonLabel);
    lonLayout->addWidget(lonEdit);
    lonLayout->addStretch();

    QHBoxLayout *altLayout = new QHBoxLayout();
    altLayout->addWidget(altLabel);
    altLayout->addWidget(altEdit);
    altLayout->addStretch();

    QHBoxLayout *headLayout = new QHBoxLayout();
    headLayout->addWidget(headLabel);
    headLayout->addWidget(headEdit);
    headLayout->addStretch();

    mainLayout->addLayout(latLayout);
    mainLayout->addLayout(lonLayout);
    mainLayout->addLayout(altLayout);
    mainLayout->addLayout(headLayout);
    mainLayout->addStretch();

    auto updateValue = [=]() {
        QJsonObject delta;
        QJsonObject geocordObj;
        geocordObj["latitude"] = latEdit->text().toDouble();
        geocordObj["longitude"] = lonEdit->text().toDouble();
        geocordObj["altitude"] = altEdit->text().toDouble();
        geocordObj["heading"] = headEdit->text().toDouble();
        geocordObj["type"] = obj["type"].toString("geocord");
        delta[fullKey] = geocordObj;

        if (inspectorRef) {
            delta["_id"] = inspectorRef->getMainID();
        }

        emit valueChanged(connectedID, name, delta);
    };

    connect(latEdit, &QLineEdit::editingFinished, this, updateValue);
    connect(lonEdit, &QLineEdit::editingFinished, this, updateValue);
    connect(altEdit, &QLineEdit::editingFinished, this, updateValue);
    connect(headEdit, &QLineEdit::editingFinished, this, updateValue);

    tableWidget->setRowHeight(row, ROW_HEIGHT * 4);
    tableWidget->setCellWidget(row, 1, this);
}

/* =============================================================================
 * FILE:         imagetemplate.cpp
 * MODULE:       Image Template Manager
 * PROJECT:      Indigenous Scenario and Sensor Simulation Toolkit (ISSST)
 * ORGANISATION: Oxygen 2 Innovation (O2I).
 * STANDARD:     RTCA DO-178C / ED-12C, DAL B
 * COVERAGE:     Branch / Decision Coverage required (100% true/false paths)
 *
 * DESCRIPTION:  Implements the ImageTemplate class which provides a widget for
 *               managing image templates. It interfaces with the Inspector
 *               panel to set up image cells in a table, maintain connected
 *               entity IDs, template names, and emit value changes when an
 *               image is modified.
 *
 * REQUIREMENTS: Implements REQ-IMAGE-010 through REQ-IMAGE-014
 *
 * AUTHOR:       Arti Rajpoot
 * REVIEWED BY:  [Reviewer Name], [Review Date] — SPR-IMAGE-001
 *
 *
 * COPYRIGHT:    Oxygen 2 Innovation (O2I). All rights reserved.
 *               Restricted circulation — defence simulation use only.
 * =============================================================================
 */
#include "imagetemplate.h"
#include "iconsdialog.h"
#include <QVBoxLayout>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QFileDialog>
#include <QDebug>
#include "GUI/Inspector/inspector.h"
#include "GUI/Inspector/inspector-styles.h"

ImageTemplate::ImageTemplate(Inspector *inspector, QWidget *parent)
    : QWidget(parent), inspectorRef(inspector) {}

void ImageTemplate::setupImageCell(int row, const QString &fullKey, const QJsonObject &obj, QTableWidget *tableWidget)
{
    QString currentPath = obj["value"].toString();

    // ── Layout: no outer margins, tight spacing so all 3 widgets fit in ROW_HEIGHT ──
    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setContentsMargins(2, 2, 2, 2);
    layout->setSpacing(2);

    // ── Thumbnail ─────────────────────────────────────────────────────────────────
    QLabel *imageLabel = new QLabel();
    imageLabel->setFixedSize(IMAGE_SIZE, IMAGE_SIZE);
    imageLabel->setScaledContents(true);
    imageLabel->setStyleSheet(InspectorStyles::ImagePreviewLabel);

    if (!currentPath.isEmpty()) {
        QPixmap pixmap(currentPath);
        if (!pixmap.isNull()) {
            imageLabel->setPixmap(
                pixmap.scaled(imageLabel->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
        } else {
            imageLabel->setText("Invalid Image");
            imageLabel->setAlignment(Qt::AlignCenter);
        }
    } else {
        imageLabel->setText("No Image");
        imageLabel->setAlignment(Qt::AlignCenter);
    }

    // ── Path line edit ────────────────────────────────────────────────────────────
    QLineEdit *lineEdit = new QLineEdit(currentPath);
    lineEdit->setStyleSheet(InspectorStyles::ImageLineEdit);
    lineEdit->setFixedHeight(28);

    // ── Browse button ─────────────────────────────────────────────────────────────
    QPushButton *browseBtn = new QPushButton("Select Image");
    browseBtn->setStyleSheet(InspectorStyles::ImageBrowseButton);
    browseBtn->setFixedHeight(28);

    layout->addWidget(imageLabel);
    layout->addWidget(lineEdit);
    layout->addWidget(browseBtn);

    // ── Browse / icons-dialog connection ─────────────────────────────────────────
    connect(browseBtn, &QPushButton::clicked, this, [=]() {
        IconsDialog dialog(this);
        if (dialog.exec() == QDialog::Accepted) {
            QString selectedPath = dialog.selectedImagePath();
            if (!selectedPath.isEmpty()) {
                lineEdit->setText(selectedPath);
                QPixmap pixmap(selectedPath);
                if (!pixmap.isNull()) {
                    imageLabel->setPixmap(
                        pixmap.scaled(imageLabel->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
                    imageLabel->setText("");
                }
                QJsonObject delta;
                QJsonObject spriteObj;
                spriteObj["value"] = selectedPath;
                spriteObj["type"]  = "image";
                delta[fullKey]     = spriteObj;
                if (inspectorRef) {
                    delta["_id"] = inspectorRef->getMainID();
                }
                emit valueChanged(connectedID, name, delta);
            }
        }
    });

    // ── Manual path edit connection ───────────────────────────────────────────────
    connect(lineEdit, &QLineEdit::editingFinished, this, [=]() {
        QString filePath = lineEdit->text();
        if (!filePath.isEmpty()) {
            QPixmap pixmap(filePath);
            if (!pixmap.isNull()) {
                imageLabel->setPixmap(
                    pixmap.scaled(imageLabel->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
                imageLabel->setText("");
            } else {
                imageLabel->setText("Invalid Image");
                imageLabel->setAlignment(Qt::AlignCenter);
            }
        } else {
            imageLabel->setText("No Image");
            imageLabel->setAlignment(Qt::AlignCenter);
        }
        QJsonObject delta;
        QJsonObject spriteObj;
        spriteObj["value"] = filePath;
        spriteObj["type"]  = "image";
        delta[fullKey]     = spriteObj;
        if (inspectorRef) {
            delta["_id"] = inspectorRef->getMainID();
        }
        emit valueChanged(connectedID, name, delta);
    });

    // ── Apply to table ────────────────────────────────────────────────────────────
    tableWidget->setRowHeight(row, ROW_HEIGHT);
    tableWidget->setCellWidget(row, 1, this);
}

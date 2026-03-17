/********************************************************************************
** Form generated from reading UI file 'inputs.ui'
**
** Created by: Qt User Interface Compiler version 5.15.13
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_INPUTS_H
#define UI_INPUTS_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QDoubleSpinBox>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QSpinBox>
#include <QtWidgets/QStackedWidget>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_Inputs
{
public:
    QHBoxLayout *horizontalLayout;
    QStackedWidget *stackedWidget;
    QWidget *page;
    QVBoxLayout *verticalLayout_2;
    QComboBox *comboBox_2;
    QWidget *page_2;
    QVBoxLayout *verticalLayout_3;
    QLabel *label;
    QWidget *page_3;
    QVBoxLayout *verticalLayout_4;
    QLabel *label_2;
    QWidget *page_4;
    QVBoxLayout *verticalLayout_5;
    QSpinBox *intfield;
    QWidget *page_5;
    QVBoxLayout *verticalLayout_6;
    QDoubleSpinBox *floatField;
    QWidget *page_6;
    QVBoxLayout *verticalLayout_7;
    QLineEdit *stringField;
    QComboBox *comboBox;

    void setupUi(QWidget *Inputs)
    {
        if (Inputs->objectName().isEmpty())
            Inputs->setObjectName(QString::fromUtf8("Inputs"));
        Inputs->resize(120, 36);
        QSizePolicy sizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
        sizePolicy.setHorizontalStretch(0);
        sizePolicy.setVerticalStretch(0);
        sizePolicy.setHeightForWidth(Inputs->sizePolicy().hasHeightForWidth());
        Inputs->setSizePolicy(sizePolicy);
        Inputs->setMaximumSize(QSize(120, 36));
        horizontalLayout = new QHBoxLayout(Inputs);
        horizontalLayout->setSpacing(0);
        horizontalLayout->setObjectName(QString::fromUtf8("horizontalLayout"));
        horizontalLayout->setSizeConstraint(QLayout::SetMinimumSize);
        horizontalLayout->setContentsMargins(0, 0, 0, 0);
        stackedWidget = new QStackedWidget(Inputs);
        stackedWidget->setObjectName(QString::fromUtf8("stackedWidget"));
        stackedWidget->setMaximumSize(QSize(100, 16777215));
        stackedWidget->setLineWidth(0);
        page = new QWidget();
        page->setObjectName(QString::fromUtf8("page"));
        QSizePolicy sizePolicy1(QSizePolicy::Preferred, QSizePolicy::Minimum);
        sizePolicy1.setHorizontalStretch(0);
        sizePolicy1.setVerticalStretch(0);
        sizePolicy1.setHeightForWidth(page->sizePolicy().hasHeightForWidth());
        page->setSizePolicy(sizePolicy1);
        verticalLayout_2 = new QVBoxLayout(page);
        verticalLayout_2->setSpacing(0);
        verticalLayout_2->setObjectName(QString::fromUtf8("verticalLayout_2"));
        verticalLayout_2->setSizeConstraint(QLayout::SetMinimumSize);
        verticalLayout_2->setContentsMargins(0, 0, 0, 0);
        comboBox_2 = new QComboBox(page);
        comboBox_2->addItem(QString());
        comboBox_2->addItem(QString());
        comboBox_2->addItem(QString());
        comboBox_2->setObjectName(QString::fromUtf8("comboBox_2"));
        comboBox_2->setMaximumSize(QSize(100, 16777215));

        verticalLayout_2->addWidget(comboBox_2);

        stackedWidget->addWidget(page);
        page_2 = new QWidget();
        page_2->setObjectName(QString::fromUtf8("page_2"));
        verticalLayout_3 = new QVBoxLayout(page_2);
        verticalLayout_3->setSpacing(0);
        verticalLayout_3->setObjectName(QString::fromUtf8("verticalLayout_3"));
        verticalLayout_3->setContentsMargins(0, 0, 0, 0);
        label = new QLabel(page_2);
        label->setObjectName(QString::fromUtf8("label"));
        label->setMaximumSize(QSize(100, 16777215));
        QFont font;
        font.setBold(true);
        label->setFont(font);
        label->setAlignment(Qt::AlignCenter);

        verticalLayout_3->addWidget(label);

        stackedWidget->addWidget(page_2);
        page_3 = new QWidget();
        page_3->setObjectName(QString::fromUtf8("page_3"));
        sizePolicy1.setHeightForWidth(page_3->sizePolicy().hasHeightForWidth());
        page_3->setSizePolicy(sizePolicy1);
        verticalLayout_4 = new QVBoxLayout(page_3);
        verticalLayout_4->setSpacing(0);
        verticalLayout_4->setObjectName(QString::fromUtf8("verticalLayout_4"));
        verticalLayout_4->setContentsMargins(0, 0, 0, 0);
        label_2 = new QLabel(page_3);
        label_2->setObjectName(QString::fromUtf8("label_2"));
        label_2->setMaximumSize(QSize(100, 16777215));
        label_2->setFont(font);
        label_2->setAlignment(Qt::AlignCenter);

        verticalLayout_4->addWidget(label_2);

        stackedWidget->addWidget(page_3);
        page_4 = new QWidget();
        page_4->setObjectName(QString::fromUtf8("page_4"));
        verticalLayout_5 = new QVBoxLayout(page_4);
        verticalLayout_5->setSpacing(0);
        verticalLayout_5->setObjectName(QString::fromUtf8("verticalLayout_5"));
        verticalLayout_5->setSizeConstraint(QLayout::SetMinimumSize);
        verticalLayout_5->setContentsMargins(0, 0, 0, 0);
        intfield = new QSpinBox(page_4);
        intfield->setObjectName(QString::fromUtf8("intfield"));
        intfield->setMaximumSize(QSize(100, 16777215));

        verticalLayout_5->addWidget(intfield);

        stackedWidget->addWidget(page_4);
        page_5 = new QWidget();
        page_5->setObjectName(QString::fromUtf8("page_5"));
        verticalLayout_6 = new QVBoxLayout(page_5);
        verticalLayout_6->setSpacing(0);
        verticalLayout_6->setObjectName(QString::fromUtf8("verticalLayout_6"));
        verticalLayout_6->setSizeConstraint(QLayout::SetMinimumSize);
        verticalLayout_6->setContentsMargins(0, 0, 0, 0);
        floatField = new QDoubleSpinBox(page_5);
        floatField->setObjectName(QString::fromUtf8("floatField"));
        floatField->setMaximumSize(QSize(100, 16777215));

        verticalLayout_6->addWidget(floatField);

        stackedWidget->addWidget(page_5);
        page_6 = new QWidget();
        page_6->setObjectName(QString::fromUtf8("page_6"));
        verticalLayout_7 = new QVBoxLayout(page_6);
        verticalLayout_7->setSpacing(0);
        verticalLayout_7->setObjectName(QString::fromUtf8("verticalLayout_7"));
        verticalLayout_7->setSizeConstraint(QLayout::SetMinimumSize);
        verticalLayout_7->setContentsMargins(0, 0, 0, 0);
        stringField = new QLineEdit(page_6);
        stringField->setObjectName(QString::fromUtf8("stringField"));
        stringField->setMaximumSize(QSize(100, 16777215));

        verticalLayout_7->addWidget(stringField);

        stackedWidget->addWidget(page_6);

        horizontalLayout->addWidget(stackedWidget);

        comboBox = new QComboBox(Inputs);
        comboBox->addItem(QString());
        comboBox->addItem(QString());
        comboBox->addItem(QString());
        comboBox->addItem(QString());
        comboBox->addItem(QString());
        comboBox->addItem(QString());
        comboBox->setObjectName(QString::fromUtf8("comboBox"));
        comboBox->setMaximumSize(QSize(20, 25));
        QFont font1;
        font1.setPointSize(8);
        comboBox->setFont(font1);

        horizontalLayout->addWidget(comboBox);


        retranslateUi(Inputs);

        stackedWidget->setCurrentIndex(1);
        comboBox->setCurrentIndex(1);


        QMetaObject::connectSlotsByName(Inputs);
    } // setupUi

    void retranslateUi(QWidget *Inputs)
    {
        Inputs->setWindowTitle(QCoreApplication::translate("Inputs", "Form", nullptr));
        comboBox_2->setItemText(0, QCoreApplication::translate("Inputs", "IsFire", nullptr));
        comboBox_2->setItemText(1, QCoreApplication::translate("Inputs", "IsLand", nullptr));
        comboBox_2->setItemText(2, QCoreApplication::translate("Inputs", "IsEnemy", nullptr));

        label->setText(QCoreApplication::translate("Inputs", "TRUE", nullptr));
        label_2->setText(QCoreApplication::translate("Inputs", "FALSE", nullptr));
        stringField->setInputMask(QString());
        stringField->setText(QCoreApplication::translate("Inputs", "TEXT", nullptr));
        comboBox->setItemText(0, QCoreApplication::translate("Inputs", "Action", nullptr));
        comboBox->setItemText(1, QCoreApplication::translate("Inputs", "TRUE", nullptr));
        comboBox->setItemText(2, QCoreApplication::translate("Inputs", "FALSE", nullptr));
        comboBox->setItemText(3, QCoreApplication::translate("Inputs", "INT Field", nullptr));
        comboBox->setItemText(4, QCoreApplication::translate("Inputs", "FLOAT Field", nullptr));
        comboBox->setItemText(5, QCoreApplication::translate("Inputs", "STRING FIELD", nullptr));

    } // retranslateUi

};

namespace Ui {
    class Inputs: public Ui_Inputs {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_INPUTS_H

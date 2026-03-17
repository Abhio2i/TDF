/********************************************************************************
** Form generated from reading UI file 'ifcondition.ui'
**
** Created by: Qt User Interface Compiler version 5.15.13
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_IFCONDITION_H
#define UI_IFCONDITION_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QFrame>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_IFCondition
{
public:
    QHBoxLayout *horizontalLayout;
    QVBoxLayout *leftinput;
    QComboBox *comboBox;
    QVBoxLayout *rightinput;
    QFrame *line;

    void setupUi(QWidget *IFCondition)
    {
        if (IFCondition->objectName().isEmpty())
            IFCondition->setObjectName(QString::fromUtf8("IFCondition"));
        IFCondition->resize(726, 26);
        QSizePolicy sizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
        sizePolicy.setHorizontalStretch(0);
        sizePolicy.setVerticalStretch(0);
        sizePolicy.setHeightForWidth(IFCondition->sizePolicy().hasHeightForWidth());
        IFCondition->setSizePolicy(sizePolicy);
        IFCondition->setMaximumSize(QSize(16777215, 26));
        horizontalLayout = new QHBoxLayout(IFCondition);
        horizontalLayout->setSpacing(0);
        horizontalLayout->setObjectName(QString::fromUtf8("horizontalLayout"));
        horizontalLayout->setSizeConstraint(QLayout::SetMinimumSize);
        horizontalLayout->setContentsMargins(0, 0, 0, 0);
        leftinput = new QVBoxLayout();
        leftinput->setSpacing(0);
        leftinput->setObjectName(QString::fromUtf8("leftinput"));
        leftinput->setSizeConstraint(QLayout::SetMinimumSize);

        horizontalLayout->addLayout(leftinput);

        comboBox = new QComboBox(IFCondition);
        comboBox->addItem(QString());
        comboBox->addItem(QString());
        comboBox->addItem(QString());
        comboBox->addItem(QString());
        comboBox->addItem(QString());
        comboBox->addItem(QString());
        comboBox->setObjectName(QString::fromUtf8("comboBox"));
        comboBox->setMaximumSize(QSize(100, 16777215));

        horizontalLayout->addWidget(comboBox);

        rightinput = new QVBoxLayout();
        rightinput->setSpacing(0);
        rightinput->setObjectName(QString::fromUtf8("rightinput"));
        rightinput->setSizeConstraint(QLayout::SetMinimumSize);
        rightinput->setContentsMargins(-1, 0, 0, -1);

        horizontalLayout->addLayout(rightinput);

        line = new QFrame(IFCondition);
        line->setObjectName(QString::fromUtf8("line"));
        line->setFrameShape(QFrame::VLine);
        line->setFrameShadow(QFrame::Sunken);

        horizontalLayout->addWidget(line);


        retranslateUi(IFCondition);

        comboBox->setCurrentIndex(4);


        QMetaObject::connectSlotsByName(IFCondition);
    } // setupUi

    void retranslateUi(QWidget *IFCondition)
    {
        IFCondition->setWindowTitle(QCoreApplication::translate("IFCondition", "Form", nullptr));
        comboBox->setItemText(0, QCoreApplication::translate("IFCondition", "Greater", nullptr));
        comboBox->setItemText(1, QCoreApplication::translate("IFCondition", "Greater/Equal", nullptr));
        comboBox->setItemText(2, QCoreApplication::translate("IFCondition", "Smaller", nullptr));
        comboBox->setItemText(3, QCoreApplication::translate("IFCondition", "Smaller/Equal", nullptr));
        comboBox->setItemText(4, QCoreApplication::translate("IFCondition", "Equal", nullptr));
        comboBox->setItemText(5, QCoreApplication::translate("IFCondition", "Not Equal", nullptr));

    } // retranslateUi

};

namespace Ui {
    class IFCondition: public Ui_IFCondition {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_IFCONDITION_H

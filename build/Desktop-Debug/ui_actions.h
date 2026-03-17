/********************************************************************************
** Form generated from reading UI file 'actions.ui'
**
** Created by: Qt User Interface Compiler version 5.15.13
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_ACTIONS_H
#define UI_ACTIONS_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QSpinBox>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_Actions
{
public:
    QHBoxLayout *horizontalLayout;
    QComboBox *action;
    QSpinBox *numberField;
    QLineEdit *stringField;

    void setupUi(QWidget *Actions)
    {
        if (Actions->objectName().isEmpty())
            Actions->setObjectName(QString::fromUtf8("Actions"));
        Actions->resize(400, 26);
        QSizePolicy sizePolicy(QSizePolicy::Preferred, QSizePolicy::Minimum);
        sizePolicy.setHorizontalStretch(0);
        sizePolicy.setVerticalStretch(0);
        sizePolicy.setHeightForWidth(Actions->sizePolicy().hasHeightForWidth());
        Actions->setSizePolicy(sizePolicy);
        Actions->setMaximumSize(QSize(16777215, 26));
        horizontalLayout = new QHBoxLayout(Actions);
        horizontalLayout->setSpacing(0);
        horizontalLayout->setObjectName(QString::fromUtf8("horizontalLayout"));
        horizontalLayout->setContentsMargins(0, 0, 0, 0);
        action = new QComboBox(Actions);
        action->addItem(QString());
        action->addItem(QString());
        action->addItem(QString());
        action->addItem(QString());
        action->addItem(QString());
        action->addItem(QString());
        action->setObjectName(QString::fromUtf8("action"));

        horizontalLayout->addWidget(action);

        numberField = new QSpinBox(Actions);
        numberField->setObjectName(QString::fromUtf8("numberField"));

        horizontalLayout->addWidget(numberField);

        stringField = new QLineEdit(Actions);
        stringField->setObjectName(QString::fromUtf8("stringField"));

        horizontalLayout->addWidget(stringField);


        retranslateUi(Actions);

        QMetaObject::connectSlotsByName(Actions);
    } // setupUi

    void retranslateUi(QWidget *Actions)
    {
        Actions->setWindowTitle(QCoreApplication::translate("Actions", "Form", nullptr));
        action->setItemText(0, QCoreApplication::translate("Actions", "None", nullptr));
        action->setItemText(1, QCoreApplication::translate("Actions", "GoHome", nullptr));
        action->setItemText(2, QCoreApplication::translate("Actions", "Print", nullptr));
        action->setItemText(3, QCoreApplication::translate("Actions", "TakeOff", nullptr));
        action->setItemText(4, QCoreApplication::translate("Actions", "Task Jump", nullptr));
        action->setItemText(5, QCoreApplication::translate("Actions", "End", nullptr));

        stringField->setText(QCoreApplication::translate("Actions", "Hello World", nullptr));
    } // retranslateUi

};

namespace Ui {
    class Actions: public Ui_Actions {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_ACTIONS_H

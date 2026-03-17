/********************************************************************************
** Form generated from reading UI file 'taskgroup.ui'
**
** Created by: Qt User Interface Compiler version 5.15.13
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_TASKGROUP_H
#define UI_TASKGROUP_H

#include <QtCore/QVariant>
#include <QtGui/QIcon>
#include <QtWidgets/QApplication>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QListWidget>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_TaskGroup
{
public:
    QVBoxLayout *verticalLayout_2;
    QLabel *MissionName;
    QHBoxLayout *horizontalLayout;
    QPushButton *addtask;
    QPushButton *run;
    QListWidget *listWidget;

    void setupUi(QWidget *TaskGroup)
    {
        if (TaskGroup->objectName().isEmpty())
            TaskGroup->setObjectName(QString::fromUtf8("TaskGroup"));
        TaskGroup->resize(803, 533);
        verticalLayout_2 = new QVBoxLayout(TaskGroup);
        verticalLayout_2->setObjectName(QString::fromUtf8("verticalLayout_2"));
        MissionName = new QLabel(TaskGroup);
        MissionName->setObjectName(QString::fromUtf8("MissionName"));
        MissionName->setMaximumSize(QSize(16777215, 20));
        QFont font;
        font.setBold(true);
        MissionName->setFont(font);

        verticalLayout_2->addWidget(MissionName);

        horizontalLayout = new QHBoxLayout();
        horizontalLayout->setSpacing(0);
        horizontalLayout->setObjectName(QString::fromUtf8("horizontalLayout"));
        horizontalLayout->setSizeConstraint(QLayout::SetMinimumSize);
        horizontalLayout->setContentsMargins(0, 0, -1, -1);
        addtask = new QPushButton(TaskGroup);
        addtask->setObjectName(QString::fromUtf8("addtask"));
        addtask->setMaximumSize(QSize(100, 16777215));
        QIcon icon(QIcon::fromTheme(QString::fromUtf8("QIcon::ThemeIcon::AppointmentNew")));
        addtask->setIcon(icon);

        horizontalLayout->addWidget(addtask);

        run = new QPushButton(TaskGroup);
        run->setObjectName(QString::fromUtf8("run"));
        run->setMaximumSize(QSize(100, 16777215));
        QIcon icon1(QIcon::fromTheme(QString::fromUtf8("QIcon::ThemeIcon::MediaPlaybackStart")));
        run->setIcon(icon1);

        horizontalLayout->addWidget(run);


        verticalLayout_2->addLayout(horizontalLayout);

        listWidget = new QListWidget(TaskGroup);
        listWidget->setObjectName(QString::fromUtf8("listWidget"));
        listWidget->setMaximumSize(QSize(16777215, 500));

        verticalLayout_2->addWidget(listWidget);


        retranslateUi(TaskGroup);

        QMetaObject::connectSlotsByName(TaskGroup);
    } // setupUi

    void retranslateUi(QWidget *TaskGroup)
    {
        TaskGroup->setWindowTitle(QCoreApplication::translate("TaskGroup", "Form", nullptr));
        MissionName->setText(QCoreApplication::translate("TaskGroup", "Mission Name", nullptr));
        addtask->setText(QCoreApplication::translate("TaskGroup", "Add Task", nullptr));
        run->setText(QCoreApplication::translate("TaskGroup", "Run", nullptr));
    } // retranslateUi

};

namespace Ui {
    class TaskGroup: public Ui_TaskGroup {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_TASKGROUP_H

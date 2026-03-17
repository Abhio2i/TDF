/********************************************************************************
** Form generated from reading UI file 'task.ui'
**
** Created by: Qt User Interface Compiler version 5.15.13
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_TASK_H
#define UI_TASK_H

#include <QtCore/QVariant>
#include <QtGui/QIcon>
#include <QtWidgets/QApplication>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QFrame>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QSpinBox>
#include <QtWidgets/QStackedWidget>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_Task
{
public:
    QVBoxLayout *verticalLayout_2;
    QLabel *label;
    QFrame *line_2;
    QHBoxLayout *horizontalLayout;
    QComboBox *command;
    QFrame *line_3;
    QStackedWidget *stackedWidget;
    QWidget *page;
    QVBoxLayout *verticalLayout_3;
    QVBoxLayout *verticalLayout;
    QWidget *page_2;
    QVBoxLayout *verticalLayout_7;
    QVBoxLayout *verticalLayout_4;
    QWidget *page_3;
    QVBoxLayout *verticalLayout_6;
    QVBoxLayout *verticalLayout_5;
    QSpinBox *JumpspinBox;
    QWidget *page_6;
    QVBoxLayout *verticalLayout_11;
    QVBoxLayout *verticalLayout_10;
    QSpinBox *waitspinBox;
    QFrame *line_4;
    QStackedWidget *progressWidget;
    QWidget *page_4;
    QVBoxLayout *verticalLayout_8;
    QLabel *label_2;
    QWidget *page_5;
    QVBoxLayout *verticalLayout_9;
    QLabel *label_3;
    QPushButton *removeButton;
    QFrame *line;

    void setupUi(QWidget *Task)
    {
        if (Task->objectName().isEmpty())
            Task->setObjectName(QString::fromUtf8("Task"));
        Task->resize(423, 54);
        QSizePolicy sizePolicy(QSizePolicy::Preferred, QSizePolicy::Minimum);
        sizePolicy.setHorizontalStretch(0);
        sizePolicy.setVerticalStretch(0);
        sizePolicy.setHeightForWidth(Task->sizePolicy().hasHeightForWidth());
        Task->setSizePolicy(sizePolicy);
        Task->setMaximumSize(QSize(16777215, 180));
        Task->setStyleSheet(QString::fromUtf8(""));
        verticalLayout_2 = new QVBoxLayout(Task);
        verticalLayout_2->setSpacing(0);
        verticalLayout_2->setObjectName(QString::fromUtf8("verticalLayout_2"));
        verticalLayout_2->setSizeConstraint(QLayout::SetMinimumSize);
        verticalLayout_2->setContentsMargins(2, 2, 2, 0);
        label = new QLabel(Task);
        label->setObjectName(QString::fromUtf8("label"));
        label->setMaximumSize(QSize(16777215, 20));

        verticalLayout_2->addWidget(label);

        line_2 = new QFrame(Task);
        line_2->setObjectName(QString::fromUtf8("line_2"));
        line_2->setFrameShape(QFrame::HLine);
        line_2->setFrameShadow(QFrame::Sunken);

        verticalLayout_2->addWidget(line_2);

        horizontalLayout = new QHBoxLayout();
        horizontalLayout->setSpacing(0);
        horizontalLayout->setObjectName(QString::fromUtf8("horizontalLayout"));
        horizontalLayout->setSizeConstraint(QLayout::SetMinimumSize);
        horizontalLayout->setContentsMargins(-1, -1, 0, -1);
        command = new QComboBox(Task);
        command->addItem(QString());
        command->addItem(QString());
        command->addItem(QString());
        command->addItem(QString());
        command->setObjectName(QString::fromUtf8("command"));
        command->setEnabled(true);
        QSizePolicy sizePolicy1(QSizePolicy::Preferred, QSizePolicy::Fixed);
        sizePolicy1.setHorizontalStretch(0);
        sizePolicy1.setVerticalStretch(0);
        sizePolicy1.setHeightForWidth(command->sizePolicy().hasHeightForWidth());
        command->setSizePolicy(sizePolicy1);
        command->setMaximumSize(QSize(100, 16777215));
        command->setInsertPolicy(QComboBox::InsertAtBottom);

        horizontalLayout->addWidget(command);

        line_3 = new QFrame(Task);
        line_3->setObjectName(QString::fromUtf8("line_3"));
        line_3->setFrameShape(QFrame::VLine);
        line_3->setFrameShadow(QFrame::Sunken);

        horizontalLayout->addWidget(line_3);

        stackedWidget = new QStackedWidget(Task);
        stackedWidget->setObjectName(QString::fromUtf8("stackedWidget"));
        stackedWidget->setMaximumSize(QSize(16777215, 26));
        page = new QWidget();
        page->setObjectName(QString::fromUtf8("page"));
        verticalLayout_3 = new QVBoxLayout(page);
        verticalLayout_3->setSpacing(0);
        verticalLayout_3->setObjectName(QString::fromUtf8("verticalLayout_3"));
        verticalLayout_3->setSizeConstraint(QLayout::SetMinimumSize);
        verticalLayout_3->setContentsMargins(0, 0, 0, 0);
        verticalLayout = new QVBoxLayout();
        verticalLayout->setSpacing(0);
        verticalLayout->setObjectName(QString::fromUtf8("verticalLayout"));
        verticalLayout->setSizeConstraint(QLayout::SetMinimumSize);

        verticalLayout_3->addLayout(verticalLayout);

        stackedWidget->addWidget(page);
        page_2 = new QWidget();
        page_2->setObjectName(QString::fromUtf8("page_2"));
        verticalLayout_7 = new QVBoxLayout(page_2);
        verticalLayout_7->setSpacing(0);
        verticalLayout_7->setObjectName(QString::fromUtf8("verticalLayout_7"));
        verticalLayout_7->setSizeConstraint(QLayout::SetMinimumSize);
        verticalLayout_7->setContentsMargins(0, 0, 0, 0);
        verticalLayout_4 = new QVBoxLayout();
        verticalLayout_4->setSpacing(0);
        verticalLayout_4->setObjectName(QString::fromUtf8("verticalLayout_4"));
        verticalLayout_4->setSizeConstraint(QLayout::SetMinimumSize);

        verticalLayout_7->addLayout(verticalLayout_4);

        stackedWidget->addWidget(page_2);
        page_3 = new QWidget();
        page_3->setObjectName(QString::fromUtf8("page_3"));
        verticalLayout_6 = new QVBoxLayout(page_3);
        verticalLayout_6->setSpacing(0);
        verticalLayout_6->setObjectName(QString::fromUtf8("verticalLayout_6"));
        verticalLayout_6->setSizeConstraint(QLayout::SetMinimumSize);
        verticalLayout_6->setContentsMargins(0, 0, 0, 0);
        verticalLayout_5 = new QVBoxLayout();
        verticalLayout_5->setSpacing(0);
        verticalLayout_5->setObjectName(QString::fromUtf8("verticalLayout_5"));
        verticalLayout_5->setSizeConstraint(QLayout::SetMinimumSize);
        JumpspinBox = new QSpinBox(page_3);
        JumpspinBox->setObjectName(QString::fromUtf8("JumpspinBox"));
        JumpspinBox->setMinimum(0);

        verticalLayout_5->addWidget(JumpspinBox);


        verticalLayout_6->addLayout(verticalLayout_5);

        stackedWidget->addWidget(page_3);
        page_6 = new QWidget();
        page_6->setObjectName(QString::fromUtf8("page_6"));
        verticalLayout_11 = new QVBoxLayout(page_6);
        verticalLayout_11->setSpacing(0);
        verticalLayout_11->setObjectName(QString::fromUtf8("verticalLayout_11"));
        verticalLayout_11->setSizeConstraint(QLayout::SetMinimumSize);
        verticalLayout_11->setContentsMargins(0, 0, 0, 0);
        verticalLayout_10 = new QVBoxLayout();
        verticalLayout_10->setSpacing(0);
        verticalLayout_10->setObjectName(QString::fromUtf8("verticalLayout_10"));
        waitspinBox = new QSpinBox(page_6);
        waitspinBox->setObjectName(QString::fromUtf8("waitspinBox"));

        verticalLayout_10->addWidget(waitspinBox);


        verticalLayout_11->addLayout(verticalLayout_10);

        stackedWidget->addWidget(page_6);

        horizontalLayout->addWidget(stackedWidget);

        line_4 = new QFrame(Task);
        line_4->setObjectName(QString::fromUtf8("line_4"));
        line_4->setFrameShape(QFrame::VLine);
        line_4->setFrameShadow(QFrame::Sunken);

        horizontalLayout->addWidget(line_4);

        progressWidget = new QStackedWidget(Task);
        progressWidget->setObjectName(QString::fromUtf8("progressWidget"));
        progressWidget->setMaximumSize(QSize(26, 26));
        page_4 = new QWidget();
        page_4->setObjectName(QString::fromUtf8("page_4"));
        verticalLayout_8 = new QVBoxLayout(page_4);
        verticalLayout_8->setSpacing(0);
        verticalLayout_8->setObjectName(QString::fromUtf8("verticalLayout_8"));
        verticalLayout_8->setContentsMargins(0, 0, 0, 0);
        label_2 = new QLabel(page_4);
        label_2->setObjectName(QString::fromUtf8("label_2"));
        label_2->setMaximumSize(QSize(20, 20));
        label_2->setPixmap(QPixmap(QString::fromUtf8(":/Image/check-mark.png")));
        label_2->setScaledContents(true);
        label_2->setAlignment(Qt::AlignCenter);

        verticalLayout_8->addWidget(label_2);

        progressWidget->addWidget(page_4);
        page_5 = new QWidget();
        page_5->setObjectName(QString::fromUtf8("page_5"));
        verticalLayout_9 = new QVBoxLayout(page_5);
        verticalLayout_9->setSpacing(0);
        verticalLayout_9->setObjectName(QString::fromUtf8("verticalLayout_9"));
        verticalLayout_9->setContentsMargins(0, 0, 0, 0);
        label_3 = new QLabel(page_5);
        label_3->setObjectName(QString::fromUtf8("label_3"));
        label_3->setMaximumSize(QSize(20, 20));
        label_3->setPixmap(QPixmap(QString::fromUtf8(":/Image/hourglass.png")));
        label_3->setScaledContents(true);
        label_3->setAlignment(Qt::AlignCenter);

        verticalLayout_9->addWidget(label_3);

        progressWidget->addWidget(page_5);

        horizontalLayout->addWidget(progressWidget);

        removeButton = new QPushButton(Task);
        removeButton->setObjectName(QString::fromUtf8("removeButton"));
        QIcon icon;
        icon.addFile(QString::fromUtf8(":/Image/removed.png"), QSize(), QIcon::Normal, QIcon::Off);
        removeButton->setIcon(icon);

        horizontalLayout->addWidget(removeButton);


        verticalLayout_2->addLayout(horizontalLayout);

        line = new QFrame(Task);
        line->setObjectName(QString::fromUtf8("line"));
        QSizePolicy sizePolicy2(QSizePolicy::Minimum, QSizePolicy::Fixed);
        sizePolicy2.setHorizontalStretch(0);
        sizePolicy2.setVerticalStretch(0);
        sizePolicy2.setHeightForWidth(line->sizePolicy().hasHeightForWidth());
        line->setSizePolicy(sizePolicy2);
        line->setFrameShape(QFrame::HLine);
        line->setFrameShadow(QFrame::Sunken);

        verticalLayout_2->addWidget(line);


        retranslateUi(Task);

        stackedWidget->setCurrentIndex(0);
        progressWidget->setCurrentIndex(1);


        QMetaObject::connectSlotsByName(Task);
    } // setupUi

    void retranslateUi(QWidget *Task)
    {
        Task->setWindowTitle(QCoreApplication::translate("Task", "Form", nullptr));
        label->setText(QCoreApplication::translate("Task", "Task1", nullptr));
        command->setItemText(0, QCoreApplication::translate("Task", "Action", nullptr));
        command->setItemText(1, QCoreApplication::translate("Task", "IF Condition", nullptr));
        command->setItemText(2, QCoreApplication::translate("Task", "TaskJump", nullptr));
        command->setItemText(3, QCoreApplication::translate("Task", "Wait", nullptr));

        label_2->setText(QString());
        label_3->setText(QString());
        removeButton->setText(QString());
    } // retranslateUi

};

namespace Ui {
    class Task: public Ui_Task {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_TASK_H

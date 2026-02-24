/* ========================================================================= */
/* File: calculator-plugin.cpp                                               */
/* ========================================================================= */

#include "GUI/Plugins/pluginregistry.h"
#include <QMenu>
#include <QAction>
#include <QInputDialog>
#include <QMessageBox>

REGISTER_PLUGIN("Calculator", [](MenuBar* mb) {

    QMenu* menu = new QMenu("Calculator", mb);
    mb->addMenu(menu);

    QAction* addAction = new QAction("Add Two Numbers", mb);
    QAction* mulAction = new QAction("Multiply Two Numbers", mb);
    QAction* divAction = new QAction("Divide Two Numbers", mb);

    menu->addAction(addAction);
    menu->addAction(mulAction);
    menu->addSeparator();
    menu->addAction(divAction);

    // Add
    QObject::connect(addAction, &QAction::triggered, []() {
        bool ok1, ok2;
        double a = QInputDialog::getDouble(nullptr, "Add", "Enter first number:", 0, -1e9, 1e9, 2, &ok1);
        double b = QInputDialog::getDouble(nullptr, "Add", "Enter second number:", 0, -1e9, 1e9, 2, &ok2);
        if (ok1 && ok2)
            QMessageBox::information(nullptr, "Result", QString("Result: %1").arg(a + b));
    });

    // Multiply
    QObject::connect(mulAction, &QAction::triggered, []() {
        bool ok1, ok2;
        double a = QInputDialog::getDouble(nullptr, "Multiply", "Enter first number:", 0, -1e9, 1e9, 2, &ok1);
        double b = QInputDialog::getDouble(nullptr, "Multiply", "Enter second number:", 0, -1e9, 1e9, 2, &ok2);
        if (ok1 && ok2)
            QMessageBox::information(nullptr, "Result", QString("Result: %1").arg(a * b));
    });

    // Divide
    QObject::connect(divAction, &QAction::triggered, []() {
        bool ok1, ok2;
        double a = QInputDialog::getDouble(nullptr, "Divide", "Enter first number:", 0, -1e9, 1e9, 2, &ok1);
        double b = QInputDialog::getDouble(nullptr, "Divide", "Enter second number:", 1, -1e9, 1e9, 2, &ok2);
        if (ok1 && ok2) {
            if (b == 0)
                QMessageBox::warning(nullptr, "Error", "Zero se divide nahi kar sakte!");
            else
                QMessageBox::information(nullptr, "Result", QString("Result: %1").arg(a / b));
        }
    });
})

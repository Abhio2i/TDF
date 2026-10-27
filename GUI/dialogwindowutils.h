#pragma once
#include <QDialog>
#include <QWidget>

inline void makeDialogResizable(QDialog* dlg)
{
    if (!dlg) return;
    dlg->setWindowFlags(Qt::Window |
                        Qt::WindowTitleHint |
                        Qt::WindowSystemMenuHint |
                        Qt::WindowMinMaxButtonsHint |
                        Qt::WindowCloseButtonHint);
    dlg->setMaximumSize(QWIDGETSIZE_MAX, QWIDGETSIZE_MAX);
}

inline void makeDialogCloseOnly(QDialog* dlg)
{
    if (!dlg) return;
    dlg->setWindowFlags(Qt::Window |
                        Qt::WindowTitleHint |
                        Qt::WindowSystemMenuHint |
                        Qt::WindowCloseButtonHint);
    dlg->setMaximumSize(QWIDGETSIZE_MAX, QWIDGETSIZE_MAX);
}

inline void makeWindowResizable(QWidget* w)
{
    if (!w) return;
    w->setWindowFlags(Qt::Window |
                      Qt::WindowTitleHint |
                      Qt::WindowSystemMenuHint |
                      Qt::WindowMinMaxButtonsHint |
                      Qt::WindowCloseButtonHint);
    w->setMaximumSize(QWIDGETSIZE_MAX, QWIDGETSIZE_MAX);
}

#include "layer_widget.h"
#include <QPainter>

LayerWidget::LayerWidget(QWidget* parent) : QWidget(parent) {
    setMinimumHeight(100);
}

LayerWidget::~LayerWidget() {}

void LayerWidget::paintEvent(QPaintEvent*) {
    QPainter painter(this);
    painter.fillRect(rect(), QColor(50, 50, 50));
    painter.setPen(Qt::white);
    painter.drawText(rect(), Qt::AlignCenter, "Layer Panel\n(Layers list and controls)");
}

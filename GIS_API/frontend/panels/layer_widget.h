#ifndef LAYER_WIDGET_H
#define LAYER_WIDGET_H

#include <QWidget>

class LayerWidget : public QWidget {
    Q_OBJECT
public:
    explicit LayerWidget(QWidget* parent = nullptr);
    ~LayerWidget();
    
protected:
    void paintEvent(QPaintEvent* event) override;
};

#endif

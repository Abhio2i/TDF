#ifndef LAYERPANEL_TEST_H
#define LAYERPANEL_TEST_H

#include <QObject>

class LayerPanel;

class TestLayerPanel : public QObject
{
    Q_OBJECT

private slots:
    void init();
    void cleanup();

    void testTreeWidgetProperties();
    void testRootItemExists();
    void testAddAndRemoveLayer();
    void testAddShapeToLayer();
    void testSetShapeDisplayName();
    void testMoveShapeToLayer();
    void testToggleLayerVisibility();
    void testActiveLayerManagement();
    void testJsonSerialization();
    void testSelectShapeInPanel();
    void testContextMenuExists();
    void testRasterLayerSupport();

private:
    LayerPanel* panel = nullptr;
};

#endif

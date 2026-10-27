#ifndef TRANSFORMMODE_H
#define TRANSFORMMODE_H

/* TransformMode enumeration section
 * Extracted from canvaswidget.h so it can be shared by IShapeHost
 * without creating a circular include (IShapeHost <-> CanvasWidget).
 */
enum TransformMode {
    Panning,
    Translate,
    Rotate,
    Scale,
    DrawTrajectory,
    DrawShape,
    PlaceBitmap,
    MeasureDistance,
    EditShape
};

#endif // TRANSFORMMODE_H

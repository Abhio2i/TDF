#ifndef IRDISPLAY_H
#define IRDISPLAY_H

#include "core/Hierarchy/EntityProfiles/sensor.h"  // For sensor profile
#include "core/Hierarchy/hierarchy.h"             // For hierarchy data structure
#include <QWidget>                                // For widget base class
#include <QVector>                                // For vector container
#include <QImage>
#include <core/Debug/profiler.h>
#include <QFileInfo>
#include <QPixmap>
#include <QtMath>
#include <QLineF>

#include "iropengldisplay.h"                      // <-- 1. Add this include
#include "core/Hierarchy/EntityProfiles/sensor.h"
#include "core/Hierarchy/hierarchy.h"

class IRDisplay : public QWidget
{
    Q_OBJECT
public:
    explicit IRDisplay(QWidget *parent = nullptr);
    // Set hierarchy instance
    void setHierarchy(Hierarchy* h) { hierarchy = h; }
    // Get size hint
    QSize sizeHint() const override;
    // Get minimum size
    QSize minimumSize() const;
    // Set radar range
    void setRange(float value) { range = value; }
    // Get height for width
    int heightForWidth(int width) const override;
    // Select entity
    void selectEntity(Entity* entity);
    // Remove entity by ID
    void RemoveEntity(QString ID);
    // Update radar display
    void updateRadar();
    // Sensor instance
    Sensor* sensor = nullptr;
    // Entity platform
    Platform* entity = nullptr;
protected:
    // Handle paint events
    void paintEvent(QPaintEvent *event) override;
    // Handle mouse move for hover detection
    void mouseMoveEvent(QMouseEvent *event) override;
    // Handle mouse leave
    void leaveEvent(QEvent *event) override;

private:
    // %%% Display Properties %%%
    // Radar range
    int range = 100;
    // Aspect ratio for display
    const float ASPECT_RATIO = 16.0/9.0;
    // Padding for display
    int padding = 9;
    // Entity ID
    QString id = "";
    // Hierarchy instance
    Hierarchy* hierarchy = nullptr;
    // Hover tracking
    int hoveredTargetIndex = -1;
    QPoint lastMousePos;

    /*----------------   Resolution End   -------------------*/
    /*------------------ EO OpenGL Start --------------------*/
protected:
    void resizeEvent(QResizeEvent *event) override; // <-- 2. Add this override
private:
    IROpenGLDisplay *glDisplay = nullptr;           // <-- 3. Add the subwidget pointer
    /*------------------- EO OpenGL End ---------------------*/

    /*------------    Custom Debugger Start    ------------*/
private:
    /*   General purpose sting For Passing   */
    QString str;

    /*  Custom enum for Selective Debugging  */
public:
    typedef enum {
        D_NULL            = 0b10000000000000,
        D_JustPrint       = 0b01000000000000,
        D_INIT            = 0b00100000000000,
        D_Connect         = 0b00010000000000,
        D_GetPayLoad      = 0b00001000000000,
        D_SetPayLoad      = 0b00000100000000,
        D_Trajectory      = 0b00000010000000,
        D_LoadInBetween   = 0b00000001000000,
        D_Resolution      = 0b00000000100000,
    }debugIRDisplay;
    Q_ENUM(debugIRDisplay)

private:
    /*   To Print Above String   */
    void debug(const QString &str,const debugIRDisplay &currentdebugType = D_JustPrint);
    /*   Variable which hold the value for
     *   Custom Debugging    */
    /*  ===> " USE ME " for debugging   <===*/
    int debugList = D_JustPrint
        //            | D_INIT
        //| D_Resolution
        ;
    /*   To find the the debugOptions inside
     *   debugType or not "Helping Function" */
    bool dbgIsAllow(const debugIRDisplay &currentdebugType);

    /*------------     Custom Debugger End     ------------*/
};


#endif // IRDISPLAY_H

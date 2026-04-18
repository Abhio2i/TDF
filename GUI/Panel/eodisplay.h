#ifndef EODISPLAY_H
#define EODISPLAY_H

#include "core/Hierarchy/EntityProfiles/sensor.h"  // For sensor profile
#include "core/Hierarchy/hierarchy.h"             // For hierarchy data structure
#include <QWidget>                                // For widget base class
#include <QVector>                                // For vector container
#include <QImage>
#include <core/Debug/profiler.h>
#include <QFileInfo>
#include <QPixmap>

class EODisplay : public QWidget
{
    Q_OBJECT
public:
    explicit EODisplay(QWidget *parent = nullptr);
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
    const double ASPECT_RATIO = 16.0/9.0;
    // Padding for display
    int padding = 9;
    // Number of radar rings
    int ringCount = 3;
    // Major tick interval
    int majorTickEvery = 30;
    // Minor ticks per major
    int minorTicksPerMajor = 5;
    // Radar green color
    QColor radarGreen = QColor(0, 255, 0);
    // Current angle
    int ang = 0;
    // List of targets
    QVector<Target> targets;
    // Entity ID
    QString id = "";
    // Hierarchy instance
    Hierarchy* hierarchy = nullptr;

    // Hover tracking
    int hoveredTargetIndex = -1;
    QPoint lastMousePos;


    struct Angles {
        double yaw;   // heading
        double pitch;
    };
    Angles vectorToAngles(const double& x,
                          const double& y,
                          const double& z);
    // %%% Drawing Methods %%%
    // Draw Vanishing point
    void drawVanishingPoint(QPainter &p, const QPoint &center);
    void drawEntity(QPainter &p, const QPoint &position, double size, int heading,int pitch);
    void drawHorizon(QPainter &p, const QPoint &center);

    enum class ImageAngles:char{
        Front  =  /* 'F' */  0 ,
        Back   =  /* 'B' */  1 ,
        Top    =  /* 'T' */  2 ,
        Bottom =  /* 'C' */  3 ,
        Left   =  /* 'L' */  4 ,
        Right  =  /* 'R' */  5
    }imgAng;
    struct imgProperties {
        float sizeConst = 1;
        float xCorr     = 0;
        float yCorr     = 0;
        imgProperties(float _sizeConst, float _xCorr, float _yCorr):
            sizeConst(_sizeConst),
            xCorr(_xCorr),
            yCorr(_yCorr){}
    };
    // std::pair<QString,imgProperties> angleImagesPath[6] = {
    //     {"../../images/air/MIRAGE2000/MIRAGE_FRONT_VIEW.png",imgProperties(1,0,0)},
    //     {"../../images/air/MIRAGE2000/MIRAGE_BACK_VIEW.png",imgProperties(1,0,0)},
    //     {"../../images/air/MIRAGE2000/MIRAGE_TOP_VIEW.png",imgProperties(1,0,0)},
    //     {"../../images/air/MIRAGE2000/MIRAGE_BOTTOM _VIEW .png",imgProperties(1,0,0)},
    //     {"../../images/air/MIRAGE2000/MIRAGE_LEFT_VIEW.png",imgProperties(1,0,0)},
    //     {"../../images/air/MIRAGE2000/MIRAGE_RIGHT_VIEW.png",imgProperties(1,0,0)}
    // };
    std::pair<QString,imgProperties> angleImagesPath[6] = {
        {"../../images/air/IR/Jet_Front.png" ,imgProperties(1,0,0)},
        {"../../images/air/IR/Jet_Back.png" ,imgProperties(1,0,0)},
        {"../../images/air/IR/Jet_Top.png" ,imgProperties(1,0,0)},
        {"../../images/air/IR/Jet_Bottom.png" ,imgProperties(1,0,0)},
        {"../../images/air/IR/Jet_Left.png" ,imgProperties(1,0,0)},
        {"../../images/air/IR/Jet_Right.png" ,imgProperties(1,0,0)}
    };
    void loadMultiDirectionalImages(QPainter &p, int heading, int pitch);
    // Draw background
    void drawBackground(QPainter &p);
    // Draw radar ring
    void drawRadarRing(QPainter &p, const QPoint &center, int outerRadius);
    // Draw ticks and labels
    void drawTicksAndLabels(QPainter &p, const QPoint &center, int outerRadius);
    // Draw concentric circles
    void drawConcentricCircles(QPainter &p, const QPoint &center, int outerRadius);
    // Draw center mark
    void drawCenterMark(QPainter &p, const QPoint &center);
    // Draw top marker
    void drawTopMarker(QPainter &p, const QPoint &center, int outerRadius);
    // Draw target and path
    void drawTargetAndPath(QPainter &painter);
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
    }debugEODisplay;
    Q_ENUM(debugEODisplay)

private:
    /*   To Print Above String   */
    void debug(const QString &str,const debugEODisplay &currentdebugType = D_JustPrint);
    /*   Variable which hold the value for
     *   Custom Debugging    */
    /*  ===> " USE ME " for debugging   <===*/
    int debugList = D_JustPrint
                        | D_INIT
        ;
    /*   To find the the debugOptions inside
     *   debugType or not "Helping Function" */
    bool dbgIsAllow(const debugEODisplay &currentdebugType);

    /*------------     Custom Debugger End     ------------*/
};


#endif // EODISPLAY_H

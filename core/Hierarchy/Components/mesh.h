#ifndef MESH_H
#define MESH_H
#include <QObject>
#include <core/Hierarchy/Struct/color.h>
#include <core/Hierarchy/Struct/vector.h>
#include <QColor>
class Mesh : public QObject
{
    Q_OBJECT
public:
    Mesh();
    bool Active;
    std::string ID;
    std::string* Sprite;
    std::string* Texture;

    QColor *color;
    QColor *fillColor;


    float lineWidth;
    bool closePath;
    std::vector<Vector*> polygen;

    void addPoint(Vector* point);
    void removePoint();
    void clear();

    void toJson();
    void fromJson();
};

#endif // MESH_H

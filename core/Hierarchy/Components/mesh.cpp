#include "mesh.h"
#include "qpixmap.h"
Mesh::Mesh() {/*Sprite = nullptr;
    Texture = nullptr;
    cacheimg = nullptr;
    color = nullptr;
    fillColor = nullptr;*/
}
Mesh::~Mesh(){
    // // 1. Raw Pointers ki heap memory clean karein
    // if (Sprite) {
    //     delete Sprite;
    //     Sprite = nullptr;
    // }

    // if (Texture) {
    //     delete Texture;
    //     Texture = nullptr;
    // }

    // if (cacheimg) {
    //     delete cacheimg;
    //     cacheimg = nullptr;
    // }

    // if (color) {
    //     delete color;
    //     color = nullptr;
    // }

    // if (fillColor) {
    //     delete fillColor;
    //     fillColor = nullptr;
    // }

    // // 2. Polygon Vector ke andar jitne dynamic Vector* pointers hain unhe free karein
    // for (Vector* point : polygen) {
    //     if (point) {
    //         delete point;
    //     }
    // }
    // polygen.clear();
    // polygen.shrink_to_fit();
}

QPixmap Mesh::getPixmap(int size) {

    if (!Sprite) {
        return QPixmap();
    }

    bool needScaling = false;

    if (lastpath != Sprite->data()) {
        lastpath = Sprite->data();
        cacheimg = QPixmap(QString::fromStdString(lastpath));
        needScaling = true;
    }

    if (cacheimg.isNull()) {
        return QPixmap();
    }

    if (needScaling || size != ImageScale) {
        ImageScale = size;
        scaledimg = cacheimg.scaled(ImageScale, ImageScale,
                                    Qt::IgnoreAspectRatio, Qt::FastTransformation);
    }

    return scaledimg;
}

void Mesh::addPoint(Vector* point) {
    polygen.push_back(point);
}

void Mesh::removePoint() {
    if (!polygen.empty()) {
        polygen.pop_back();
    }
}

void Mesh::clear() {
    polygen.clear();
}

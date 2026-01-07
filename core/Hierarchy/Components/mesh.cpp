#include "mesh.h"
#include "qpixmap.h"
Mesh::Mesh() {}


QPixmap* Mesh::getPixmap(int size){
    if(lastpath != Sprite->data()){
        lastpath = Sprite->data();
        cacheimg = new QPixmap(QString::fromStdString(lastpath));
        scaledimg = cacheimg->scaled(1 * ImageScale, 1 * ImageScale,
                                     Qt::IgnoreAspectRatio, Qt::FastTransformation);
    }
    if(size != ImageScale){
        ImageScale = size;
        scaledimg = cacheimg->scaled(1 * ImageScale, 1 * ImageScale,
                                 Qt::IgnoreAspectRatio, Qt::FastTransformation);
    }

    return &scaledimg;

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

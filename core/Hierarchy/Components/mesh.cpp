#include "mesh.h"

Mesh::Mesh() {}


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

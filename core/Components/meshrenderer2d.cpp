#include "meshrenderer2d.h"
#include <QColor>
MeshRenderer2D::MeshRenderer2D() {
    Active = true;
    color = new QColor(Qt::red);
    color2 = std::make_shared<QColor>(Qt::blue); // ya koi bhi value
    Sprite = new std::string(":/texture/Images/Texture/car.png");
    Texture = new std::string(":/texture/Images/Texture/waall.jpg");
    Mesh *mesh = new Mesh();
    mesh->color = color;
    mesh->Sprite = Sprite;
    mesh->Texture = Texture;
    mesh->lineWidth = 2;
    mesh->closePath = true;
    mesh->addPoint(new Vector(0,10,0));
    mesh->addPoint(new Vector(10,-10,0));
    mesh->addPoint(new Vector(-10,-10,0));
    Meshes.push_back(mesh);
}


QJsonObject MeshRenderer2D::toJson() const {
    QJsonObject obj;
    obj["active"] = Active;
    obj["id"] = QString::fromStdString(ID);
    QJsonObject colorObj;
    colorObj["type"] = "color";
    colorObj["value"] = color->name();
    obj["color"] = colorObj;

    QJsonObject spriteObj;
    spriteObj["type"] = "image";
    spriteObj["value"] = Sprite->data();
    obj["sprite"] = spriteObj;

    QJsonObject textureObj;
    textureObj["type"] = "image";
    textureObj["value"] = Texture->data();
    obj["texture"] = textureObj;
    return obj;
}

void MeshRenderer2D::fromJson(const QJsonObject& obj) {
    if (obj.contains("active"))
        Active = obj["active"].toBool();

    if (obj.contains("id"))
        ID = obj["id"].toString().toStdString();

    if (obj.contains("sprite")){
        Sprite->clear();
        QJsonObject spriteObj = obj["sprite"].toObject();
        if (spriteObj.contains("value"))
            Sprite->append(spriteObj["value"].toString().toStdString());
    }

    if (obj.contains("texture")){
        Texture->clear();
        QJsonObject textureObj = obj["texture"].toObject();
        if (textureObj.contains("value"))
            Texture->append(textureObj["value"].toString().toStdString());
    }

    if (obj.contains("color")){
        QJsonObject colorObj = obj["color"].toObject();
        if (colorObj.contains("value")){
            color2 = std::make_shared<QColor>(colorObj["value"].toString());
            color->setRed(color2->red());
            color->setGreen(color2->green());
            color->setBlue(color2->blue());
            color->setAlpha(color2->alpha());
        }
    }
}

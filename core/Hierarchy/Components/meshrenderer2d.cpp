

#include "meshrenderer2d.h"
#include "qjsondocument.h"
#include <QColor>
#include <QDebug>

MeshRenderer2D::MeshRenderer2D():Component(nullptr) {
    Active = true;
    color = new QColor(Qt::red);
    color2 = std::make_shared<QColor>(Qt::blue);
    Sprite = new std::string(":/texture/images/Texture/fighterjet.png");
    Texture = new std::string(":/model/airplane/Model/Airplane/11803_Airplane_body_diff.jpg");
    Model3d = new std::string(":/model/airplane/Model/Airplane/11803_Airplane_v1_l1.obj");
    Mesh *mesh = new Mesh();
    mesh->color = color;
    mesh->Sprite = Sprite;
    mesh->Texture = Texture;
    mesh->lineWidth = 2;
    mesh->closePath = true;
    // mesh->addPoint(new Vector(0, 10, 0));
    // mesh->addPoint(new Vector(10, -10, 0));
    // mesh->addPoint(new Vector(-10, -10, 0));
    Meshes.push_back(mesh);
}

void MeshRenderer2D::addSubComponent(std::string name, QString data1, QString data2, QJsonObject data3){

}

void MeshRenderer2D::removeSubComponent(std::string ID){

}

void MeshRenderer2D::updateSubComponent(std::string ID, const QJsonObject& obj){

}

QJsonObject MeshRenderer2D::getsubComponentData(std::string ID) const{
    return QJsonObject();
}

QJsonObject MeshRenderer2D::toJson() const {
    QJsonObject obj;
    obj["active"] = Active;
    obj["id"] = QString::fromStdString(ID);
    obj["type"] = "component";
    QJsonObject colorObj;
    colorObj["type"] = "color";
    colorObj["value"] = color->name();
    obj["color"] = colorObj;

    QJsonObject spriteObj;
    spriteObj["type"] = "image";
    spriteObj["value"] = QString::fromStdString(*Sprite);
    obj["sprite"] = spriteObj;

    QJsonObject textureObj;
    textureObj["type"] = "image";
    textureObj["value"] = QString::fromStdString(*Texture);
    obj["texture"] = textureObj;

    QJsonObject model3dObj;
    textureObj["type"] = "Section";
    textureObj["value"] = QString::fromStdString(*Model3d);
    obj["Model3d"] = textureObj;

    QJsonObject AddParameters = AdditionalParameters;
    AddParameters["type"] = "Section";
    obj["AdditionalParameters"] = AddParameters;

    //qDebug() << "MeshRenderer2D::toJson output:" << QJsonDocument(obj).toJson(QJsonDocument::Compact);
    return obj;
}

void MeshRenderer2D::fromJson(const QJsonObject& obj) {
    //qDebug() << "MeshRenderer2D::fromJson input:" << QJsonDocument(obj).toJson(QJsonDocument::Compact);

    // Standard fields
    if (obj.contains("active"))
        Active = obj["active"].toBool();
    // if (obj.contains("id"))
    //     ID = obj["id"].toString().toStdString();
    if (obj.contains("sprite")) {
        Sprite->clear();
        QJsonObject spriteObj = obj["sprite"].toObject();
        if (spriteObj.contains("value"))
            Sprite->append(spriteObj["value"].toString().toStdString());
    }
    if (obj.contains("texture")) {
        Texture->clear();
        QJsonObject textureObj = obj["texture"].toObject();
        if (textureObj.contains("value"))
            Texture->append(textureObj["value"].toString().toStdString());
    }

    if (obj.contains("Model3d")) {
        Model3d->clear();
        QJsonObject model3dObj = obj["Model3d"].toObject();
        if (model3dObj.contains("value"))
            Model3d->append(model3dObj["value"].toString().toStdString());
    }

    if (obj.contains("color")) {
        QJsonObject colorObj = obj["color"].toObject();
        if (colorObj.contains("value")) {
            color2 = std::make_shared<QColor>(colorObj["value"].toString());
            color->setRed(color2->red());
            color->setGreen(color2->green());
            color->setBlue(color2->blue());
            color->setAlpha(color2->alpha());
        }
    }

    if(obj.contains("AdditionalParameters")){
        AdditionalParameters = obj["AdditionalParameters"].toObject();
    }



    //qDebug() << "MeshRenderer2D::fromJson customParameters:" << QJsonDocument(customParameters).toJson(QJsonDocument::Compact);
}

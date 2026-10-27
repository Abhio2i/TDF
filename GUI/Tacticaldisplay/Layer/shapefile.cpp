#include "shapefile.h"
#include "GUI/Tacticaldisplay/Layer/ILayerHost.h"
#include "GUI/Tacticaldisplay/Shape/mesh_entry.h"
#include <core/Hierarchy/Components/mesh.h>
#include <core/Hierarchy/Struct/vector.h>
#include <QFile>
#include <QFileInfo>
#include <QDataStream>
#include <QDate>
#include <QVector3D>
#include <QPointF>
#include <QtMath>
#include <cmath>
#include <limits>
// CHAITANYA JAIN CHANGE:
// Why: Extracted Shapefile export functionality from LayerPanel into a standalone module.

#include <qgsvectorlayer.h>
#include <qgsfeature.h>
#include <qgsgeometry.h>
bool Shapefile::importLayer(ILayerHost* host,
                            const QString& filePath,
                            const QString& layerName)
{
    if (!host)
        return false;

    QgsVectorLayer* layer = new QgsVectorLayer(filePath, "temp", "ogr");

    if (!layer->isValid())
    {
        delete layer;
        return false;
    }

    QgsFeatureIterator it = layer->getFeatures();
    QgsFeature feat;

    while (it.nextFeature(feat))
    {
        QgsGeometry geom = feat.geometry();

        if (geom.isNull())
            continue;

        //---------------------------------------------
        // Read attributes
        //---------------------------------------------

        QString featName;

        QVariant nameAttr = feat.attribute("NAME");
        if (!nameAttr.isValid())
            nameAttr = feat.attribute("name");
        if (!nameAttr.isValid())
            nameAttr = feat.attribute("Name");
        if (!nameAttr.isValid())
            nameAttr = feat.attribute("id");

        featName = nameAttr.toString();

        QString type =
            feat.attribute("TYPE").toString();

        QString textValue =
            feat.attribute("TEXT").toString();

        //---------------------------------------------
        // POINT / TEXT
        //---------------------------------------------

        if (geom.type() == Qgis::GeometryType::Point)
        {
            QVector<QgsPointXY> points;

            if (geom.isMultipart())
            {
                points = geom.asMultiPoint();
            }
            else
            {
                points.append(geom.asPoint());
            }

            for (const QgsPointXY& pt : points)
            {
                QVector3D position(pt.x(), pt.y(), 0);

                std::vector<QVector3D> offsets;
                offsets.push_back(QVector3D(0,0,0));

                QString shapeType = "Point";
                QString shapeName = featName;

                if (type.compare("Text", Qt::CaseInsensitive) == 0)
                {
                    shapeType = "Text";

                    if (!textValue.isEmpty())
                        shapeName = textValue;
                }

                host->registerImportedShape(
                    layerName,
                    shapeType,
                    shapeName,
                    position,
                    offsets,
                    QVector3D(1,1,1),
                    true);
            }
        }

        //---------------------------------------------
        // LINE
        //---------------------------------------------

        else if (geom.type() == Qgis::GeometryType::Line)
        {
            QVector<QgsPolylineXY> lines;

            if (geom.isMultipart())
                lines = geom.asMultiPolyline();
            else
                lines.append(geom.asPolyline());

            for (const QgsPolylineXY& line : lines)
            {
                if (line.size() < 2)
                    continue;

                double cx = 0;
                double cy = 0;

                for (const QgsPointXY& p : line)
                {
                    cx += p.x();
                    cy += p.y();
                }

                cx /= line.size();
                cy /= line.size();

                QVector3D position(cx, cy, 0);

                std::vector<QVector3D> offsets;

                for (const QgsPointXY& p : line)
                {
                    offsets.push_back(
                        QVector3D(
                            p.x() - cx,
                            p.y() - cy,
                            0));
                }

                host->registerImportedShape(
                    layerName,
                    "Line",
                    featName,
                    position,
                    offsets,
                    QVector3D(1,1,1),
                    false);
            }
        }

        //---------------------------------------------
        // POLYGON
        //---------------------------------------------

        else if (geom.type() == Qgis::GeometryType::Polygon)
        {
            QVector<QgsPolygonXY> polygons;

            if (geom.isMultipart())
                polygons = geom.asMultiPolygon();
            else
                polygons.append(geom.asPolygon());

            for (const QgsPolygonXY& polygon : polygons)
            {
                if (polygon.isEmpty())
                    continue;

                const QgsPolylineXY& ring = polygon.first();

                if (ring.size() < 3)
                    continue;

                int count = ring.size();

                if (ring.first() == ring.last())
                    count--;

                double cx = 0;
                double cy = 0;

                for (int i = 0; i < count; i++)
                {
                    cx += ring[i].x();
                    cy += ring[i].y();
                }

                cx /= count;
                cy /= count;

                QVector3D position(cx, cy, 0);

                std::vector<QVector3D> offsets;

                for (int i = 0; i < count; i++)
                {
                    offsets.push_back(
                        QVector3D(
                            ring[i].x() - cx,
                            ring[i].y() - cy,
                            0));
                }

                host->registerImportedShape(
                    layerName,
                    "Polygon",
                    featName,
                    position,
                    offsets,
                    QVector3D(1,1,1),
                    true);
            }
        }
    }

    delete layer;

    return true;
}
bool Shapefile::exportLayer(ILayerHost* host,
                            const QString& layerName,
                            const QStringList& shapeIds,
                            const QString& filePath)
{
    if (!host)
        return false;

    QFileInfo info(filePath);

    QString dir  = info.absolutePath();
    QString base = info.completeBaseName();

    QVector<const MeshEntry*> polygons;
    QVector<const MeshEntry*> lines;
    QVector<const MeshEntry*> points;

    std::vector<MeshEntry> meshes =
        host->getMeshEntriesForShapes(shapeIds);

    for (MeshEntry& entry : meshes)
    {
        if (!entry.position)
            continue;

        QString n = entry.name;

        //------------------------------------
        // POLYGONS
        //------------------------------------

        if (n.startsWith("TempPolygon",Qt::CaseInsensitive) ||
            n.startsWith("Polygon",Qt::CaseInsensitive) ||
            n.startsWith("TempRectangle",Qt::CaseInsensitive) ||
            n.startsWith("Rectangle",Qt::CaseInsensitive) ||
            n.startsWith("TempCircle",Qt::CaseInsensitive) ||
            n.startsWith("Circle",Qt::CaseInsensitive))
        {
            polygons.push_back(&entry);
            continue;
        }

        //------------------------------------
        // LINES
        //------------------------------------

        if (n.startsWith("TempPolyline",Qt::CaseInsensitive) ||
            n.startsWith("Polyline",Qt::CaseInsensitive) ||
            n.startsWith("Line",Qt::CaseInsensitive))
        {
            lines.push_back(&entry);
            continue;
        }

        //------------------------------------
        // POINTS
        //------------------------------------

        if (n.startsWith("TempPoint", Qt::CaseInsensitive) ||
            n.startsWith("Point", Qt::CaseInsensitive) ||

            n.startsWith("TempText", Qt::CaseInsensitive) ||
            n.startsWith("Text", Qt::CaseInsensitive))
        {
            points.push_back(&entry);
            continue;
        }

        //------------------------------------
        // Ignore Text / Bitmap / Raster
        //------------------------------------
    }

    if (polygons.isEmpty() &&
        lines.isEmpty() &&
        points.isEmpty())
    {
        return false;
    }

    bool ok = true;

    if (!polygons.isEmpty())
    {
        ok &= exportShapesByType(
            polygons,
            dir + "/" + base + "_polygons.shp",
            layerName,
            5);
    }

    if (!lines.isEmpty())
    {
        ok &= exportShapesByType(
            lines,
            dir + "/" + base + "_lines.shp",
            layerName,
            3);
    }

    if (!points.isEmpty())
    {
        ok &= exportShapesByType(
            points,
            dir + "/" + base + "_points.shp",
            layerName,
            1);
    }

    return ok;
}
bool Shapefile::exportShapesByType(const QVector<const MeshEntry*>& shapes,
                                   const QString& filePath,
                                   const QString& layerName,
                                   int shapeType)
{
    Q_UNUSED(layerName);

    QFileInfo fi(filePath);

    QString baseName = fi.completeBaseName();
    QString dirPath  = fi.absolutePath();

    QString shpPath = dirPath + "/" + baseName + ".shp";
    QString shxPath = dirPath + "/" + baseName + ".shx";
    QString dbfPath = dirPath + "/" + baseName + ".dbf";
    QString prjPath = dirPath + "/" + baseName + ".prj";

    QFile::remove(shpPath);
    QFile::remove(shxPath);
    QFile::remove(dbfPath);
    QFile::remove(prjPath);

    QFile shpFile(shpPath);
    if(!shpFile.open(QIODevice::WriteOnly))
        return false;

    QFile shxFile(shxPath);
    if(!shxFile.open(QIODevice::WriteOnly))
    {
        shpFile.close();
        return false;
    }

    QDataStream shpStream(&shpFile);
    QDataStream shxStream(&shxFile);

    shpStream.setByteOrder(QDataStream::BigEndian);
    shxStream.setByteOrder(QDataStream::BigEndian);

    //---------------------------------------------
    // Global Bounding Box
    //---------------------------------------------

    double minX =  180.0;
    double minY =   90.0;
    double maxX = -180.0;
    double maxY =  -90.0;

    QVector<QByteArray> records;
    QVector<const MeshEntry*> validShapes;
    QVector<qint32> offsets;

    qint32 currentOffset = 50;

    for(const MeshEntry* entry : shapes)
    {
        if(!entry)
            continue;

        if(!entry->position ||
            !entry->rotation ||
            !entry->mesh)
            continue;

        QByteArray rec =
            createShapefileRecord(
                entry,
                shapeType,
                minX,
                minY,
                maxX,
                maxY);

        if(rec.isEmpty())
            continue;

        validShapes.push_back(entry);

        offsets.push_back(currentOffset);

        records.push_back(rec);

        currentOffset += 4 + rec.size()/2;
    }

    if(records.isEmpty())
    {
        shpFile.close();
        shxFile.close();

        QFile::remove(shpPath);
        QFile::remove(shxPath);

        return false;
    }

    //---------------------------------------------
    // Safety
    //---------------------------------------------

    if(minX>maxX)
    {
        minX=-180;
        maxX=180;
    }

    if(minY>maxY)
    {
        minY=-90;
        maxY=90;
    }

    //---------------------------------------------
    // Headers
    //---------------------------------------------

    qint32 shpLength=currentOffset;
    qint32 shxLength=50+(records.size()*4);

    writeShapefileHeader(
        shpStream,
        shpLength,
        shapeType,
        minX,
        minY,
        maxX,
        maxY);

    writeShapefileHeader(
        shxStream,
        shxLength,
        shapeType,
        minX,
        minY,
        maxX,
        maxY);

    //---------------------------------------------
    // SHP Records
    //---------------------------------------------

    for(int i=0;i<records.size();i++)
    {
        shpStream<<(qint32)(i+1);

        shpStream<<(qint32)(records[i].size()/2);

        shpStream.writeRawData(
            records[i].constData(),
            records[i].size());
    }

    //---------------------------------------------
    // SHX Records
    //---------------------------------------------

    for(int i=0;i<offsets.size();i++)
    {
        shxStream<<offsets[i];
        shxStream<<(qint32)(records[i].size()/2);
    }

    shpFile.close();
    shxFile.close();

    //---------------------------------------------
    // DBF
    //---------------------------------------------

    writeDBF(dbfPath, validShapes);

    //---------------------------------------------
    // PRJ
    //---------------------------------------------

    QFile prj(prjPath);

    if(prj.open(QIODevice::WriteOnly|QIODevice::Text))
    {
        QTextStream out(&prj);

        out <<
            "GEOGCS[\"WGS 84\","
            "DATUM[\"WGS_1984\","
            "SPHEROID[\"WGS 84\",6378137,298.257223563]],"
            "PRIMEM[\"Greenwich\",0],"
            "UNIT[\"degree\",0.0174532925199433]]";

        prj.close();
    }

    return true;
}
void Shapefile::writeShapefileHeader(QDataStream& stream,
                                     qint32 fileLength,
                                     int shapeType,
                                     double minX,
                                     double minY,
                                     double maxX,
                                     double maxY)
{
    //-----------------------------------------
    // Sanity check
    //-----------------------------------------

    if (minX > maxX) {
        minX = -180.0;
        maxX = 180.0;
    }

    if (minY > maxY) {
        minY = -90.0;
        maxY = 90.0;
    }

    //-----------------------------------------
    // Big Endian Section
    //-----------------------------------------

    stream.setByteOrder(QDataStream::BigEndian);

    // File Code
    stream << (qint32)9994;

    // 5 unused integers
    for (int i = 0; i < 5; ++i)
        stream << (qint32)0;

    // File length (16-bit words)
    stream << fileLength;

    //-----------------------------------------
    // Little Endian Section
    //-----------------------------------------

    stream.setByteOrder(QDataStream::LittleEndian);

    // Version
    stream << (qint32)1000;

    // Shape Type
    stream << (qint32)shapeType;

    // Bounding Box
    stream << minX;
    stream << minY;
    stream << maxX;
    stream << maxY;

    // Z Range
    stream << 0.0;
    stream << 0.0;

    // M Range
    stream << 0.0;
    stream << 0.0;

    //-----------------------------------------
    // Restore Big Endian
    //-----------------------------------------

    stream.setByteOrder(QDataStream::BigEndian);
}
QByteArray Shapefile::createShapefileRecord(const MeshEntry* entry,
                                            int shapeType,
                                            double& minX,
                                            double& minY,
                                            double& maxX,
                                            double& maxY)
{
    QByteArray record;

    if (!entry)
        return record;

    if (!entry->position || !entry->rotation)
        return record;

    if (shapeType != 1)
    {
        if (!entry->mesh)
            return record;

        if (entry->mesh->polygen.empty() &&
            !entry->name.startsWith("TempCircle"))
            return record;
    }

    QDataStream stream(&record, QIODevice::WriteOnly);
    stream.setByteOrder(QDataStream::LittleEndian);

    QString shapeName = entry->name;

    QVector3D center = *entry->position;

    float rotation = entry->rotation->z();
    float cosFwd = std::cos(rotation);
    float sinFwd = std::sin(rotation);

    QVector<QPointF> points;

    //--------------------------------------------------
    // Circle
    //--------------------------------------------------

    if (shapeName.startsWith("TempCircle"))
    {
        double radius = entry->size ?
                            qAbs(entry->size->x()) :
                            0.001;

        const int numPoints = 180;

        for (int i = 0; i < numPoints; ++i)
        {
            double angle = (2.0 * M_PI * i) / numPoints;

            double latRadius = radius;
            double lonRadius = radius / std::cos(qDegreesToRadians(center.y()));

            double lon = center.x() + lonRadius * std::cos(angle);
            double lat = center.y() + latRadius * std::sin(angle);

            points.append(QPointF(lon, lat));
        }

        // close polygon ONLY ONCE
        points.append(points.first());

        if (!points.isEmpty())
            points.append(points.first());
    }

    //--------------------------------------------------
    // Point
    //--------------------------------------------------

    else if (shapeName.startsWith("TempPoint") ||
             shapeName.startsWith("Point") ||

             shapeName.startsWith("TempText") ||
             shapeName.startsWith("Text"))
    {
        points.append(QPointF(center.x(), center.y()));
    }

    //--------------------------------------------------
    // Polygon / Polyline / Rectangle
    //--------------------------------------------------

    else
    {
        for (Vector* v : entry->mesh->polygen)
        {
            if (!v)
                continue;

            double worldX =
                v->x * cosFwd -
                v->y * sinFwd;

            double worldY =
                v->x * sinFwd +
                v->y * cosFwd;

            double lon =
                center.x() +
                worldX;

            double lat =
                center.y() +
                worldY;

            points.append(QPointF(lon, lat));
        }

        if (shapeType == 5 &&
            points.size() > 2)
        {
            if (points.first() != points.last())
                points.append(points.first());
        }
    }

    //--------------------------------------------------
    // Validation
    //--------------------------------------------------

    if (shapeType == 1)
    {
        if (points.size() != 1)
            return QByteArray();
    }

    if (shapeType == 3)
    {
        if (points.size() < 2)
            return QByteArray();
    }

    if (shapeType == 5)
    {
        if (points.size() < 4)
            return QByteArray();
    }

    //--------------------------------------------------
    // Global Bounding Box
    //--------------------------------------------------

    for (const QPointF& pt : points)
    {
        minX = qMin(minX, pt.x());
        maxX = qMax(maxX, pt.x());

        minY = qMin(minY, pt.y());
        maxY = qMax(maxY, pt.y());
    }

    //--------------------------------------------------
    // Write Record
    //--------------------------------------------------

    stream << (qint32)shapeType;

    //--------------------------------------------------
    // Point
    //--------------------------------------------------

    if (shapeType == 1)
    {
        stream << points[0].x();
        stream << points[0].y();
    }

    //--------------------------------------------------
    // Polyline / Polygon
    //--------------------------------------------------

    else
    {
        double recMinX = std::numeric_limits<double>::max();
        double recMinY = std::numeric_limits<double>::max();

        double recMaxX = std::numeric_limits<double>::lowest();
        double recMaxY = std::numeric_limits<double>::lowest();

        for (const QPointF& pt : points)
        {
            recMinX = qMin(recMinX, pt.x());
            recMaxX = qMax(recMaxX, pt.x());

            recMinY = qMin(recMinY, pt.y());
            recMaxY = qMax(recMaxY, pt.y());
        }

        stream << recMinX;
        stream << recMinY;
        stream << recMaxX;
        stream << recMaxY;

        stream << (qint32)1;
        stream << (qint32)points.size();

        stream << (qint32)0;

        for (const QPointF& pt : points)
        {
            stream << pt.x();
            stream << pt.y();
        }
    }

    return record;
}
bool Shapefile::writeDBF(const QString& filePath, const QVector<const MeshEntry*>& shapes)
{
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly)) {
        return false;
    }

    QDataStream stream(&file);
    stream.setByteOrder(QDataStream::LittleEndian);

    // DBF header
    stream << (quint8)0x03;  // Version (dBASE III)

    // Date (YY MM DD)
    QDate today = QDate::currentDate();
    stream << (quint8)(today.year() - 1900);
    stream << (quint8)today.month();
    stream << (quint8)today.day();

    // Number of records
    stream << (quint32)shapes.size();

    // Header size (32 bytes header + 32 bytes per field + 1 terminator)
    quint16 headerSize = 32 + (2 * 32) + 1;
    stream << headerSize;

    // Record size (1 delete flag + field sizes)
    quint16 recordSize = 1 + 50 + 20;  // NAME(50) + TYPE(20)
    stream << recordSize;

    // Reserved (20 bytes)
    for (int i = 0; i < 20; i++) {
        stream << (quint8)0;
    }

    // Field descriptors
    writeDBFFieldDescriptor(stream, "NAME", 'C', 50);
    writeDBFFieldDescriptor(stream, "TYPE", 'C', 20);

    // Header terminator
    stream << (quint8)0x0D;

    // Records
    for (const MeshEntry* entry : shapes) {
        // Delete flag (space = not deleted)
        stream << (quint8)0x20;

        // NAME field (50 chars)
        QString name = entry->name.leftJustified(50, ' ', true);
        stream.writeRawData(name.toLatin1().constData(), 50);

        // TYPE field (20 chars)
        QString type = getShapeType(*entry).leftJustified(20, ' ', true);
        stream.writeRawData(type.toLatin1().constData(), 20);
        QString text = "";

        if (entry->name.startsWith("TempText") ||
            entry->name.startsWith("Text"))
        {
            text = entry->text;
        }

        text = text.leftJustified(100,' ',true);

        stream.writeRawData(text.toLatin1().constData(),100);
    }

    // End of file marker
    stream << (quint8)0x1A;

    file.close();
    return true;
}

void Shapefile::writeDBFFieldDescriptor(QDataStream& stream, const QString& name,
                                        char type, int length)
{
    // Field name (11 bytes, null-terminated)
    QByteArray fieldName = name.toLatin1().leftJustified(11, '\0', true);
    stream.writeRawData(fieldName.constData(), 11);

    // Field type (C=Character, N=Numeric, L=Logical, D=Date)
    stream << (quint8)type;

    // Reserved (4 bytes)
    stream << (quint32)0;

    // Field length
    stream << (quint8)length;

    // Decimal count
    stream << (quint8)0;

    // Reserved (14 bytes)
    for (int i = 0; i < 14; i++) {
        stream << (quint8)0;
    }
}
QString Shapefile::getShapeType(const MeshEntry& entry)
{
    QString name = entry.name;

    if (name.startsWith("TempCircle") ||
        name.startsWith("Circle"))
        return "Circle";

    if (name.startsWith("TempRectangle") ||
        name.startsWith("Rectangle"))
        return "Rectangle";

    if (name.startsWith("TempPolygon") ||
        name.startsWith("Polygon"))
        return "Polygon";

    if (name.startsWith("TempPolyline") ||
        name.startsWith("Line") ||
        name.startsWith("Polyline"))
        return "Line";

    if (name.startsWith("TempPoint") ||
        name.startsWith("Point") ||
        name.startsWith("TempText") ||
        name.startsWith("Text"))
        return "Point";

    if (name.startsWith("TempBitmap") ||
        name.startsWith("Bitmap") ||
        name.startsWith("UserImage") ||
        name.startsWith("Image"))
        return "Point";

    return "Unknown";
}

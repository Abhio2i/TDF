#include "builtin_layer_formats.h"
#include "layer_format_registry.h"
#include "backend/formats/geojson_simple.h"
#include <QFile>
#include <QFileInfo>
#include <QTextStream>
#include <QDataStream>
#include <QXmlStreamWriter>
#include <QXmlStreamReader>
#include <QRegularExpression>
#include <QDate>
#include <QtEndian>
#include <QDebug>
#include <cmath>

static QList<QPointF> ringFromShape(const ShapeData& s)
{
    QList<QPointF> ring;
    if (s.type == QLatin1String("circle") && s.radius > 0) {
        for (int i = 0; i <= 36; ++i) {
            double a = (i * 360.0 / 36) * M_PI / 180.0;
            ring.append(QPointF(s.longitude + s.radius * std::cos(a),
                                s.latitude + s.radius * std::sin(a)));
        }
    } else if (!s.points.isEmpty()) {
        ring = s.points.toList();
    } else if (s.type == QLatin1String("rectangle") && s.width > 0 && s.height > 0) {
        double hw = s.width / 2.0, hh = s.height / 2.0;
        ring << QPointF(s.longitude - hw, s.latitude + hh)
             << QPointF(s.longitude + hw, s.latitude + hh)
             << QPointF(s.longitude + hw, s.latitude - hh)
             << QPointF(s.longitude - hw, s.latitude - hh);
    }
    return ring;
}

static void centroidOf(const QList<QPointF>& pts, double& lon, double& lat)
{
    lon = lat = 0;
    if (pts.isEmpty()) return;
    for (const QPointF& p : pts) { lon += p.x(); lat += p.y(); }
    lon /= pts.size(); lat /= pts.size();
}

class GeoJsonFormat : public ILayerFormat {
public:
    QString id() const override { return QStringLiteral("geojson"); }
    QString filter() const override { return QStringLiteral("GeoJSON (*.geojson)"); }
    QStringList extensions() const override { return {QStringLiteral("geojson"), QStringLiteral("json")}; }
    bool exportShapes(const QList<ShapeData>& shapes, const QString& path) const override {
        QStringList ids;
        for (const ShapeData& s : shapes) if (!s.id.isEmpty()) ids.append(s.id);
        return !ids.isEmpty() && GeoJsonSimple::exportShapes(ids, path);
    }
    QList<ShapeData> importShapes(const QString& path) const override {
        return GeoJsonSimple::importShapes(path);
    }
};

class CsvFormat : public ILayerFormat {
public:
    QString id() const override { return QStringLiteral("csv"); }
    QString filter() const override { return QStringLiteral("CSV (*.csv)"); }
    QStringList extensions() const override { return {QStringLiteral("csv")}; }
    bool exportShapes(const QList<ShapeData>& shapes, const QString& path) const override {
        QFile f(path);
        if (!f.open(QIODevice::WriteOnly|QIODevice::Text|QIODevice::Truncate)) return false;
        QTextStream out(&f);
        out << "Name,Type,Longitude,Latitude,Geometry_WKT\n";
        for (const ShapeData& s : shapes) {
            QString name = s.text.isEmpty() ? s.type : s.text;
            if (name.contains(',') || name.contains('"'))
                name = "\"" + QString(name).replace("\"","\"\"") + "\"";
            QString wkt = toWkt(s);
            if (wkt.contains(',') || wkt.contains('"'))
                wkt = "\"" + wkt.replace("\"","\"\"") + "\"";
            out << name << ',' << s.type << ','
                << QString::number(s.longitude,'f',8) << ','
                << QString::number(s.latitude,'f',8) << ',' << wkt << '\n';
        }
        return true;
    }
    QList<ShapeData> importShapes(const QString& path) const override {
        QList<ShapeData> result;
        QFile f(path);
        if (!f.open(QIODevice::ReadOnly|QIODevice::Text)) return result;
        QTextStream in(&f);
        auto split = [](const QString& line) {
            QStringList fields; QString cur; bool q=false;
            for (int i=0;i<line.size();++i) {
                QChar c=line[i];
                if (c=='"'){ if(q && i+1<line.size() && line[i+1]=='"'){cur+='"';++i;} else q=!q; }
                else if (c==',' && !q){ fields<<cur; cur.clear(); }
                else cur+=c;
            }
            fields<<cur; return fields;
        };
        QStringList h = split(in.readLine());
        auto idx = [&](const QStringList& names){ for(const QString& n:names){int i=h.indexOf(n); if(i>=0)return i;} return -1; };
        int wi=idx({"Geometry_WKT","WKT","geom","geometry"});
        int ni=idx({"Name","name"}); int ti=idx({"Type","type"});
        if (wi<0) return result;
        while (!in.atEnd()) {
            QStringList fields = split(in.readLine());
            if (fields.size()<=wi) continue;
            ShapeData s = fromWkt(fields[wi].trimmed(), ti>=0&&ti<fields.size()?fields[ti]:QString());
            if (s.type.isEmpty()) continue;
            if (ni>=0&&ni<fields.size()) s.text=fields[ni];
            result.append(s);
        }
        return result;
    }
private:
    static QString toWkt(const ShapeData& s) {
        if (s.type=="point"||s.type=="text")
            return QString("POINT(%1 %2)").arg(s.longitude,0,'f',8).arg(s.latitude,0,'f',8);
        if (s.type=="line" && s.points.size()>=2) {
            QStringList p; for(const QPointF& pt:s.points) p<<QString("%1 %2").arg(pt.x(),0,'f',8).arg(pt.y(),0,'f',8);
            return "LINESTRING("+p.join(',')+")";
        }
        QList<QPointF> ring=ringFromShape(s);
        if (ring.size()>=3) {
            if(ring.first()!=ring.last()) ring.append(ring.first());
            QStringList p; for(const QPointF& pt:ring) p<<QString("%1 %2").arg(pt.x(),0,'f',8).arg(pt.y(),0,'f',8);
            return "POLYGON(("+p.join(',')+"))";
        }
        return QString("POINT(%1 %2)").arg(s.longitude,0,'f',8).arg(s.latitude,0,'f',8);
    }
    static ShapeData fromWkt(const QString& wkt, const QString& hint) {
        ShapeData s; s.color=Qt::red; s.lineWidth=2;
        QString u=wkt.toUpper(), body=wkt;
        int a=body.indexOf('('), b=body.lastIndexOf(')');
        if(a>=0&&b>a) body=body.mid(a+1,b-a-1);
        body.replace('(',' ').replace(')',' ');
        QList<QPointF> pts;
        for (const QString& pair: body.split(',', Qt::SkipEmptyParts)) {
            QStringList xy=pair.trimmed().split(QRegularExpression("\\s+"), Qt::SkipEmptyParts);
            if(xy.size()>=2) pts.append(QPointF(xy[0].toDouble(), xy[1].toDouble()));
        }
        if(pts.isEmpty()) return s;
        if(u.contains("POINT")){ s.type=hint.compare("text",Qt::CaseInsensitive)==0?"text":"point"; s.longitude=pts[0].x(); s.latitude=pts[0].y(); }
        else if(u.contains("LINESTRING")){ s.type="line"; s.points=pts.toVector(); centroidOf(pts,s.longitude,s.latitude); }
        else if(u.contains("POLYGON")){
            if(pts.size()>1&&pts.first()==pts.last()) pts.removeLast();
            if(hint.compare("circle",Qt::CaseInsensitive)==0){ s.type="circle"; centroidOf(pts,s.longitude,s.latitude); s.radius=std::abs(pts[0].x()-s.longitude); }
            else { s.type=hint.compare("rectangle",Qt::CaseInsensitive)==0?"rectangle":"polygon"; s.points=pts.toVector(); centroidOf(pts,s.longitude,s.latitude); }
        }
        return s;
    }
};

class KmlFormat : public ILayerFormat {
public:
    QString id() const override { return QStringLiteral("kml"); }
    QString filter() const override { return QStringLiteral("KML (*.kml)"); }
    QStringList extensions() const override { return {QStringLiteral("kml")}; }
    bool exportShapes(const QList<ShapeData>& shapes, const QString& path) const override {
        QFile f(path); if(!f.open(QIODevice::WriteOnly|QIODevice::Text|QIODevice::Truncate)) return false;
        QXmlStreamWriter xml(&f); xml.setAutoFormatting(true);
        xml.writeStartDocument(); xml.writeStartElement("kml");
        xml.writeAttribute("xmlns","http://www.opengis.net/kml/2.2");
        xml.writeStartElement("Document");
        for(const ShapeData& s: shapes){
            xml.writeStartElement("Placemark");
            xml.writeTextElement("name", s.text.isEmpty()?s.type:s.text);
            xml.writeTextElement("description", s.type);
            if(s.type=="point"||s.type=="text"||s.type=="bitmap"){
                xml.writeStartElement("Point");
                xml.writeTextElement("coordinates", QString("%1,%2,0").arg(s.longitude,0,'f',8).arg(s.latitude,0,'f',8));
                xml.writeEndElement();
            } else if(s.type=="line" && s.points.size()>=2){
                QStringList p; for(const QPointF& pt:s.points) p<<QString("%1,%2,0").arg(pt.x(),0,'f',8).arg(pt.y(),0,'f',8);
                xml.writeStartElement("LineString"); xml.writeTextElement("coordinates", p.join(' ')); xml.writeEndElement();
            } else {
                QList<QPointF> ring=ringFromShape(s);
                if(ring.size()>=3){
                    if(ring.first()!=ring.last()) ring.append(ring.first());
                    QStringList p; for(const QPointF& pt:ring) p<<QString("%1,%2,0").arg(pt.x(),0,'f',8).arg(pt.y(),0,'f',8);
                    xml.writeStartElement("Polygon"); xml.writeStartElement("outerBoundaryIs");
                    xml.writeStartElement("LinearRing"); xml.writeTextElement("coordinates", p.join(' '));
                    xml.writeEndElement(); xml.writeEndElement(); xml.writeEndElement();
                }
            }
            xml.writeEndElement();
        }
        xml.writeEndElement(); xml.writeEndElement(); xml.writeEndDocument();
        return true;
    }
    QList<ShapeData> importShapes(const QString& path) const override {
        QList<ShapeData> result;
        QFile f(path); if(!f.open(QIODevice::ReadOnly|QIODevice::Text)) return result;
        QXmlStreamReader xml(&f); QString name;
        auto parse=[](const QString& t){
            QList<QPointF> pts;
            for(const QString& tok: t.split(QRegularExpression("[\\s]+"), Qt::SkipEmptyParts)){
                QStringList c=tok.split(',');
                if(c.size()>=2) pts.append(QPointF(c[0].toDouble(),c[1].toDouble()));
            }
            return pts;
        };
        while(!xml.atEnd()){
            xml.readNext();
            if(!xml.isStartElement()) continue;
            if(xml.name()==QLatin1String("name")) name=xml.readElementText();
            else if(xml.name()==QLatin1String("Point")||xml.name()==QLatin1String("LineString")||xml.name()==QLatin1String("Polygon")){
                QString tag=xml.name().toString();
                ShapeData s; s.color=Qt::red; s.lineWidth=2; if(!name.isEmpty()) s.text=name;
                while(!(xml.isEndElement()&&xml.name()==tag)){
                    xml.readNext();
                    if(xml.isStartElement()&&xml.name()==QLatin1String("coordinates")){
                        auto pts=parse(xml.readElementText());
                        if(tag=="Point"&&!pts.isEmpty()){ s.type="point"; s.longitude=pts[0].x(); s.latitude=pts[0].y(); }
                        else if(tag=="LineString"){ s.type="line"; s.points=pts.toVector(); centroidOf(pts,s.longitude,s.latitude); }
                        else { if(pts.size()>1&&pts.first()==pts.last()) pts.removeLast(); s.type="polygon"; s.points=pts.toVector(); centroidOf(pts,s.longitude,s.latitude); }
                    }
                }
                if(!s.type.isEmpty()) result.append(s);
            }
        }
        return result;
    }
};

class GmlFormat : public ILayerFormat {
public:
    QString id() const override { return QStringLiteral("gml"); }
    QString filter() const override { return QStringLiteral("GML (*.gml)"); }
    QStringList extensions() const override { return {QStringLiteral("gml")}; }
    bool exportShapes(const QList<ShapeData>& shapes, const QString& path) const override {
        QFile f(path); if(!f.open(QIODevice::WriteOnly|QIODevice::Text|QIODevice::Truncate)) return false;
        QTextStream out(&f);
        out << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
        out << "<gml:FeatureCollection xmlns:gml=\"http://www.opengis.net/gml\">\n";
        for(const ShapeData& s: shapes){
            out << "  <gml:featureMember><Feature>\n";
            out << "    <gml:name>" << (s.text.isEmpty()?s.type:s.text).toHtmlEscaped() << "</gml:name>\n";
            auto c=[](double x,double y){return QString("%1,%2").arg(x,0,'f',10).arg(y,0,'f',10);};
            if(s.type=="point"||s.type=="text")
                out << "    <gml:Point srsName=\"CRS:84\"><gml:coordinates>" << c(s.longitude,s.latitude) << "</gml:coordinates></gml:Point>\n";
            else if(s.type=="line"&&s.points.size()>=2){
                QStringList p; for(const QPointF& pt:s.points) p<<c(pt.x(),pt.y());
                out << "    <gml:LineString srsName=\"CRS:84\"><gml:coordinates>" << p.join(' ') << "</gml:coordinates></gml:LineString>\n";
            } else {
                QList<QPointF> ring=ringFromShape(s);
                if(ring.size()>=3){
                    if(ring.first()!=ring.last()) ring.append(ring.first());
                    QStringList p; for(const QPointF& pt:ring) p<<c(pt.x(),pt.y());
                    out << "    <gml:Polygon srsName=\"CRS:84\"><gml:outerBoundaryIs><gml:LinearRing><gml:coordinates>"
                        << p.join(' ') << "</gml:coordinates></gml:LinearRing></gml:outerBoundaryIs></gml:Polygon>\n";
                }
            }
            out << "  </Feature></gml:featureMember>\n";
        }
        out << "</gml:FeatureCollection>\n";
        return true;
    }
    QList<ShapeData> importShapes(const QString& path) const override {
        QList<ShapeData> result;
        QFile f(path); if(!f.open(QIODevice::ReadOnly|QIODevice::Text)) return result;
        QXmlStreamReader xml(&f); QString name, geom;
        while(!xml.atEnd()){
            xml.readNext();
            if(xml.isStartElement()){
                QString tag=xml.name().toString();
                if(tag=="name") name=xml.readElementText();
                else if(tag=="Point"||tag=="LineString"||tag=="Polygon") geom=tag;
                else if(tag=="coordinates"||tag=="posList"||tag=="pos"){
                    bool comma=(tag=="coordinates");
                    QString text=xml.readElementText();
                    QList<QPointF> pts;
                    if(comma){
                        for(const QString& tok: text.split(QRegularExpression("[\\s]+"), Qt::SkipEmptyParts)){
                            QStringList c=tok.split(',');
                            if(c.size()>=2) pts.append(QPointF(c[0].toDouble(),c[1].toDouble()));
                        }
                    } else {
                        QStringList v=text.split(QRegularExpression("[\\s]+"), Qt::SkipEmptyParts);
                        for(int i=0;i+1<v.size();i+=2) pts.append(QPointF(v[i].toDouble(),v[i+1].toDouble()));
                    }
                    ShapeData s; s.color=Qt::red; s.lineWidth=2; if(!name.isEmpty()) s.text=name;
                    if(geom=="Point"&&!pts.isEmpty()){ s.type="point"; s.longitude=pts[0].x(); s.latitude=pts[0].y(); result.append(s); }
                    else if(geom=="LineString"&&pts.size()>=2){ s.type="line"; s.points=pts.toVector(); centroidOf(pts,s.longitude,s.latitude); result.append(s); }
                    else if(geom=="Polygon"&&pts.size()>=3){
                        if(pts.first()==pts.last()) pts.removeLast();
                        s.type="polygon"; s.points=pts.toVector(); centroidOf(pts,s.longitude,s.latitude); result.append(s);
                    }
                }
            } else if(xml.isEndElement()){
                QString tag=xml.name().toString();
                if(tag=="Point"||tag=="LineString"||tag=="Polygon") geom.clear();
            }
        }
        return result;
    }
};

class ShapefileFormat : public ILayerFormat {
public:
    QString id() const override { return QStringLiteral("shapefile"); }
    QString filter() const override { return QStringLiteral("Shapefile (*.shp)"); }
    QStringList extensions() const override { return {QStringLiteral("shp")}; }
    bool exportShapes(const QList<ShapeData>& shapes, const QString& filePath) const override {
        if (shapes.isEmpty()) return false;
        QList<ShapeData> points, lines, polygons;
        for (const ShapeData& s : shapes) {
            if (s.type=="point"||s.type=="text"||s.type=="bitmap") points.append(s);
            else if (s.type=="line") lines.append(s);
            else polygons.append(s);
        }
        QFileInfo fi(filePath);
        QString dir=fi.absolutePath(), base=fi.completeBaseName();
        if (base.endsWith("_points")||base.endsWith("_lines")||base.endsWith("_polygons")) {
            int u=base.lastIndexOf('_'); if(u>0) base=base.left(u);
        }
        bool ok=false;
        if(!points.isEmpty()) ok|=writeSet(points, dir+"/"+base+(lines.isEmpty()&&polygons.isEmpty()?".shp":"_points.shp"), 1);
        if(!lines.isEmpty()) ok|=writeSet(lines, dir+"/"+base+(points.isEmpty()&&polygons.isEmpty()?".shp":"_lines.shp"), 3);
        if(!polygons.isEmpty()) ok|=writeSet(polygons, dir+"/"+base+(points.isEmpty()&&lines.isEmpty()?".shp":"_polygons.shp"), 5);
        return ok;
    }
    QList<ShapeData> importShapes(const QString& path) const override { return readShp(path); }
private:
    static void writeHeader(QDataStream& st, qint32 words, int type, double minX,double minY,double maxX,double maxY) {
        st.setByteOrder(QDataStream::BigEndian);
        st<<(qint32)9994; for(int i=0;i<5;++i) st<<(qint32)0; st<<words;
        st.setByteOrder(QDataStream::LittleEndian);
        st<<(qint32)1000<<(qint32)type<<minX<<minY<<maxX<<maxY<<0.0<<0.0<<0.0<<0.0;
        st.setByteOrder(QDataStream::BigEndian);
    }
    static QByteArray recPoint(const ShapeData& s, double& minX,double& minY,double& maxX,double& maxY) {
        QByteArray r; QDataStream st(&r,QIODevice::WriteOnly); st.setByteOrder(QDataStream::LittleEndian);
        st<<(qint32)1<<s.longitude<<s.latitude;
        minX=qMin(minX,s.longitude); maxX=qMax(maxX,s.longitude);
        minY=qMin(minY,s.latitude); maxY=qMax(maxY,s.latitude); return r;
    }
    static QByteArray recPoly(const ShapeData& s, int type, double& minX,double& minY,double& maxX,double& maxY) {
        QList<QPointF> pts = (type==3)?s.points.toList():ringFromShape(s);
        if(type==5&&pts.size()>=3&&pts.first()!=pts.last()) pts.append(pts.first());
        if((type==3&&pts.size()<2)||(type==5&&pts.size()<4)) return {};
        double r0=1e300,r1=1e300,r2=-1e300,r3=-1e300;
        for(const QPointF& p:pts){ r0=qMin(r0,p.x()); r2=qMax(r2,p.x()); r1=qMin(r1,p.y()); r3=qMax(r3,p.y()); }
        minX=qMin(minX,r0); maxX=qMax(maxX,r2); minY=qMin(minY,r1); maxY=qMax(maxY,r3);
        QByteArray r; QDataStream st(&r,QIODevice::WriteOnly); st.setByteOrder(QDataStream::LittleEndian);
        st<<(qint32)type<<r0<<r1<<r2<<r3<<(qint32)1<<(qint32)pts.size()<<(qint32)0;
        for(const QPointF& p:pts) st<<p.x()<<p.y(); return r;
    }
    static bool writeDbf(const QString& path, const QList<ShapeData>& shapes) {
        QFile f(path); if(!f.open(QIODevice::WriteOnly)) return false;
        QDataStream st(&f); st.setByteOrder(QDataStream::LittleEndian);
        st<<(quint8)0x03;
        QDate d=QDate::currentDate();
        st<<(quint8)(d.year()-1900)<<(quint8)d.month()<<(quint8)d.day();
        st<<(quint32)shapes.size()<<(quint16)(32+64+1)<<(quint16)(1+50+20);
        for(int i=0;i<20;++i) st<<(quint8)0;
        auto field=[&](const char* n, char t, int len){
            QByteArray name=QByteArray(n).leftJustified(11,'\0',true);
            st.writeRawData(name.constData(),11);
            st<<(quint8)t<<(quint32)0<<(quint8)len<<(quint8)0;
            for(int i=0;i<14;++i) st<<(quint8)0;
        };
        field("NAME",'C',50); field("TYPE",'C',20); st<<(quint8)0x0D;
        for(const ShapeData& s:shapes){
            st<<(quint8)0x20;
            QByteArray name=(s.text.isEmpty()?s.type:s.text).toLatin1().leftJustified(50,' ',true);
            st.writeRawData(name.constData(),50);
            QByteArray type=s.type.toLatin1().leftJustified(20,' ',true);
            st.writeRawData(type.constData(),20);
        }
        st<<(quint8)0x1A; return true;
    }
    static void writePrj(const QString& path) {
        QFile f(path); if(!f.open(QIODevice::WriteOnly|QIODevice::Text)) return;
        QTextStream(&f) << "GEOGCS[\"WGS 84\",DATUM[\"WGS_1984\",SPHEROID[\"WGS 84\",6378137,298.257223563]],PRIMEM[\"Greenwich\",0],UNIT[\"degree\",0.0174532925199433]]";
    }
    static bool writeSet(const QList<ShapeData>& shapes, const QString& shpPath, int type) {
        QFileInfo fi(shpPath);
        QString base=fi.absolutePath()+"/"+fi.completeBaseName();
        QFile::remove(shpPath); QFile::remove(base+".shx"); QFile::remove(base+".dbf"); QFile::remove(base+".prj");
        QVector<QByteArray> records; QVector<qint32> offsets;
        double minX=180,minY=90,maxX=-180,maxY=-90; qint32 off=50;
        for(const ShapeData& s:shapes){
            QByteArray rec=(type==1)?recPoint(s,minX,minY,maxX,maxY):recPoly(s,type,minX,minY,maxX,maxY);
            if(rec.isEmpty()) continue;
            offsets.append(off); records.append(rec); off+=4+rec.size()/2;
        }
        if(records.isEmpty()) return false;
        if(minX>maxX){minX=-180;maxX=180;} if(minY>maxY){minY=-90;maxY=90;}
        QFile shp(shpPath), shx(base+".shx");
        if(!shp.open(QIODevice::WriteOnly)||!shx.open(QIODevice::WriteOnly)) return false;
        QDataStream ss(&shp), xs(&shx);
        writeHeader(ss, off, type, minX,minY,maxX,maxY);
        writeHeader(xs, 50+records.size()*4, type, minX,minY,maxX,maxY);
        for(int i=0;i<records.size();++i){
            ss.setByteOrder(QDataStream::BigEndian);
            ss<<(qint32)(i+1)<<(qint32)(records[i].size()/2);
            ss.writeRawData(records[i].constData(), records[i].size());
            xs.setByteOrder(QDataStream::BigEndian);
            xs<<offsets[i]<<(qint32)(records[i].size()/2);
        }
        writeDbf(base+".dbf", shapes); writePrj(base+".prj"); return true;
    }
    static QStringList readDbfNames(const QString& path) {
        QStringList names; QFile f(path); if(!f.open(QIODevice::ReadOnly)) return names;
        QByteArray d=f.readAll(); if(d.size()<32) return names;
        quint32 n=qFromLittleEndian<quint32>(reinterpret_cast<const uchar*>(d.constData()+4));
        quint16 hs=qFromLittleEndian<quint16>(reinterpret_cast<const uchar*>(d.constData()+8));
        quint16 rs=qFromLittleEndian<quint16>(reinterpret_cast<const uchar*>(d.constData()+10));
        int nl=50; if(hs>32){ nl=(quint8)d[32+16]; if(nl<=0||nl>254) nl=50; }
        for(quint32 i=0;i<n;++i){ int o=hs+int(i)*rs; if(o+1+nl>d.size()) break; names<<QString::fromLatin1(d.mid(o+1,nl)).trimmed(); }
        return names;
    }
    static QList<ShapeData> readShp(const QString& path) {
        QList<ShapeData> result;
        QFile f(path); if(!f.open(QIODevice::ReadOnly)) return result;
        QDataStream st(&f); st.setByteOrder(QDataStream::BigEndian);
        qint32 code=0; st>>code; if(code!=9994) return result;
        for(int i=0;i<5;++i){ qint32 u; st>>u; } qint32 fl=0; st>>fl;
        st.setByteOrder(QDataStream::LittleEndian);
        qint32 ver=0,stype=0; st>>ver>>stype;
        double a,b,c,d,e,g,h,i; st>>a>>b>>c>>d>>e>>g>>h>>i;
        QStringList names=readDbfNames(QFileInfo(path).absolutePath()+"/"+QFileInfo(path).completeBaseName()+".dbf");
        int idx=0;
        while(!st.atEnd() && f.pos()+8<=f.size()){
            st.setByteOrder(QDataStream::BigEndian);
            qint32 rn=0,cl=0; st>>rn>>cl; if(cl<=0) break;
            st.setByteOrder(QDataStream::LittleEndian);
            qint32 type=0; st>>type;
            ShapeData s; s.color=Qt::red; s.lineWidth=2;
            if(idx<names.size()) s.text=names[idx];
            if(type==1){ double x=0,y=0; st>>x>>y; s.type="point"; s.longitude=x; s.latitude=y; result.append(s); }
            else if(type==3||type==5){
                double x0,y0,x1,y1; st>>x0>>y0>>x1>>y1;
                qint32 np=0,npts=0; st>>np>>npts;
                QVector<qint32> parts(np); for(int j=0;j<np;++j) st>>parts[j];
                QList<QPointF> pts; for(int j=0;j<npts;++j){ double x=0,y=0; st>>x>>y; pts.append(QPointF(x,y)); }
                int start=parts.isEmpty()?0:parts[0];
                int end=(parts.size()>1)?parts[1]:npts;
                QList<QPointF> ring=pts.mid(start,end-start);
                if(type==5&&ring.size()>1&&ring.first()==ring.last()) ring.removeLast();
                s.type=(type==3)?"line":"polygon"; s.points=ring.toVector();
                centroidOf(ring,s.longitude,s.latitude);
                if((type==3&&ring.size()>=2)||(type==5&&ring.size()>=3)) result.append(s);
            } else { int rem=cl-2; if(rem>0) f.seek(f.pos()+rem*2); }
            ++idx;
        }
        return result;
    }
};

class FlatGeobufStub : public ILayerFormat {
public:
    QString id() const override { return QStringLiteral("flatgeobuf"); }
    QString filter() const override { return QStringLiteral("FlatGeobuf (*.fgb)"); }
    QStringList extensions() const override { return {QStringLiteral("fgb")}; }
    bool exportShapes(const QList<ShapeData>&, const QString&) const override {
        qWarning() << "FlatGeobuf: register plugin with id flatgeobuf"; return false;
    }
    QList<ShapeData> importShapes(const QString&) const override { return {}; }
};

void registerBuiltInLayerFormats()
{
    auto* reg = LayerFormatRegistry::instance();
    if (reg->format(QStringLiteral("geojson"))) return;
    reg->registerFormat(new GeoJsonFormat());
    reg->registerFormat(new KmlFormat());
    reg->registerFormat(new GmlFormat());
    reg->registerFormat(new ShapefileFormat());
    reg->registerFormat(new FlatGeobufStub());
    reg->registerFormat(new CsvFormat());
}

#ifndef LAYER_FORMAT_REGISTRY_H
#define LAYER_FORMAT_REGISTRY_H

#include "ilayer_format.h"
#include <QObject>
#include <QMap>
#include <QStringList>
#include <QVector>

class LayerFormatRegistry : public QObject
{
    Q_OBJECT
public:
    static LayerFormatRegistry* instance();

    void registerFormat(ILayerFormat* format);
    void unregisterFormat(const QString& id);

    ILayerFormat* format(const QString& id) const;
    ILayerFormat* formatForExtension(const QString& ext) const;
    ILayerFormat* formatForPath(const QString& filePath) const;

    QList<ILayerFormat*> allFormats() const;
    QStringList formatIds() const;

    QString exportFilterString() const;
    QString importFilterString() const;
    QString idFromFilter(const QString& selectedFilter) const;

signals:
    void formatRegistered(const QString& id);
    void formatUnregistered(const QString& id);

private:
    explicit LayerFormatRegistry(QObject* parent = nullptr);
    static LayerFormatRegistry* m_instance;
    QMap<QString, ILayerFormat*> m_formats;
    QVector<QString> m_order;
};

void registerBuiltInLayerFormats();

#endif // LAYER_FORMAT_REGISTRY_H

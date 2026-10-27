#include "layer_format_registry.h"
#include <QFileInfo>
#include <QDebug>

LayerFormatRegistry* LayerFormatRegistry::m_instance = nullptr;

LayerFormatRegistry* LayerFormatRegistry::instance()
{
    if (!m_instance)
        m_instance = new LayerFormatRegistry();
    return m_instance;
}

LayerFormatRegistry::LayerFormatRegistry(QObject* parent)
    : QObject(parent)
{
}

void LayerFormatRegistry::registerFormat(ILayerFormat* format)
{
    if (!format || format->id().isEmpty()) {
        qWarning() << "LayerFormatRegistry: invalid format";
        delete format;
        return;
    }
    const QString id = format->id().toLower();
    if (m_formats.contains(id)) {
        delete m_formats.take(id);
        m_order.removeAll(id);
    }
    m_formats.insert(id, format);
    m_order.append(id);
    emit formatRegistered(id);
}

void LayerFormatRegistry::unregisterFormat(const QString& id)
{
    const QString key = id.toLower();
    if (!m_formats.contains(key))
        return;
    delete m_formats.take(key);
    m_order.removeAll(key);
    emit formatUnregistered(key);
}

ILayerFormat* LayerFormatRegistry::format(const QString& id) const
{
    return m_formats.value(id.toLower(), nullptr);
}

ILayerFormat* LayerFormatRegistry::formatForExtension(const QString& ext) const
{
    const QString e = ext.toLower().trimmed();
    for (ILayerFormat* f : m_formats) {
        if (f->extensions().contains(e))
            return f;
    }
    return nullptr;
}

ILayerFormat* LayerFormatRegistry::formatForPath(const QString& filePath) const
{
    return formatForExtension(QFileInfo(filePath).suffix());
}

QList<ILayerFormat*> LayerFormatRegistry::allFormats() const
{
    QList<ILayerFormat*> list;
    for (const QString& id : m_order) {
        if (m_formats.contains(id))
            list.append(m_formats[id]);
    }
    return list;
}

QStringList LayerFormatRegistry::formatIds() const
{
    return QStringList(m_order.begin(), m_order.end());
}

QString LayerFormatRegistry::exportFilterString() const
{
    QStringList parts;
    for (ILayerFormat* f : allFormats()) {
        if (f->canExport())
            parts << f->filter();
    }
    return parts.join(QStringLiteral(";;"));
}

QString LayerFormatRegistry::importFilterString() const
{
    QStringList parts;
    for (ILayerFormat* f : allFormats()) {
        if (f->canImport())
            parts << f->filter();
    }
    parts << QStringLiteral("All Files (*)");
    return parts.join(QStringLiteral(";;"));
}

QString LayerFormatRegistry::idFromFilter(const QString& selectedFilter) const
{
    for (ILayerFormat* f : allFormats()) {
        if (f->filter() == selectedFilter)
            return f->id();
    }
    for (ILayerFormat* f : allFormats()) {
        for (const QString& ext : f->extensions()) {
            if (selectedFilter.contains(QStringLiteral("*.") + ext, Qt::CaseInsensitive))
                return f->id();
        }
    }
    return QString();
}

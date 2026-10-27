#ifndef SHAPE_HISTORY_API_H
#define SHAPE_HISTORY_API_H

#include <QObject>
#include <QString>
#include <QJsonObject>
#include <QMap>
#include <QVector>


class ShapeHistoryApi : public QObject {
    Q_OBJECT

public:
    static ShapeHistoryApi* instance();
    static constexpr int kMaxSnapshots = 10;

    Q_INVOKABLE void pushSnapshot(const QString& shapeId);
    Q_INVOKABLE void pushSnapshot(const QString& shapeId, const QJsonObject& props);
    Q_INVOKABLE bool hasHistory(const QString& shapeId) const;
    Q_INVOKABLE int historyCount(const QString& shapeId) const;
    Q_INVOKABLE QJsonObject lastSnapshot(const QString& shapeId) const;
    Q_INVOKABLE void clear(const QString& shapeId);
    Q_INVOKABLE void clearAll();
    Q_INVOKABLE bool isPreviewActive() const;
    Q_INVOKABLE QString previewShapeId() const;
    Q_INVOKABLE QJsonObject previewGeometry() const;
    Q_INVOKABLE bool startPreview(const QString& shapeId);
    Q_INVOKABLE void hidePreview();
    Q_INVOKABLE bool restoreHistory(const QString& shapeId);

signals:
    void historyChanged(const QString& shapeId);
    void previewChanged(bool active, const QString& shapeId);

private:
    explicit ShapeHistoryApi(QObject* parent = nullptr);
    static ShapeHistoryApi* m_instance;

    static QJsonObject stripId(const QJsonObject& obj);
    static bool roughlyEqual(const QJsonObject& a, const QJsonObject& b);
    void appendSnapshot(const QString& shapeId, const QJsonObject& snap);

    QMap<QString, QVector<QJsonObject>> m_history;
    QString m_previewId;
    QJsonObject m_previewGeom;
};

#endif

#ifndef AIRBASE_API_H
#define AIRBASE_API_H

#include <QObject>
#include <QPixmap>
#include <QString>
#include <QVector>
#include <QPointF>


class AirbaseApi : public QObject {
    Q_OBJECT

public:
    static AirbaseApi* instance();
    Q_INVOKABLE void initialize();
    Q_INVOKABLE void loadAirbaseData();
    Q_INVOKABLE void setVisible(bool visible);
    Q_INVOKABLE void toggleVisibility();
    Q_INVOKABLE bool isVisible() const { return m_visible; }
    Q_INVOKABLE int count() const { return m_positions.size(); }
    Q_INVOKABLE QVector<QPointF> getPositions() const { return m_positions; }
    QPixmap icon() const { return m_icon; }
    Q_INVOKABLE void setIconPath(const QString& path);

signals:
    void visibilityChanged(bool visible);
    void dataLoaded(int count);

private:
    explicit AirbaseApi(QObject* parent = nullptr);
    ~AirbaseApi() override;
    static AirbaseApi* m_instance;
    void loadIcon();
    bool m_visible = false;
    bool m_loaded = false;
    QVector<QPointF> m_positions;
    QString m_iconPath = QStringLiteral(":/icons/images/airbase.png");
    QPixmap m_icon;
};

#endif

#pragma once
#include <QDialog>
#include <QLineEdit>
#include <QListWidget>
#include <QScrollArea>
#include <QLabel>
#include <QPushButton>
#include <QStringList>
#include <QPaintEvent>
#include <QSettings>

struct PluginInfo {
    int         id;
    QString     name;
    QString     version;
    QString     author;
    QString     description;
    QStringList tags;
    QString     status;
    QString     iconColor;
     QString     filePath;
};

class PluginManagerDialog : public QDialog
{
    Q_OBJECT
public:
    explicit PluginManagerDialog(QWidget *parent = nullptr);
    QList<PluginInfo> getPlugins() const { return plugins; }
    static QList<PluginInfo> loadSavedPlugins() {
        QSettings settings("MyApp", "PluginManager");
        int size = settings.beginReadArray("plugins");
        QList<PluginInfo> list;
        for (int i = 0; i < size; ++i) {
            settings.setArrayIndex(i);
            PluginInfo p;
            p.id       = settings.value("id").toInt();
            p.name     = settings.value("name").toString();
            p.status   = settings.value("status").toString();
            p.filePath = settings.value("filePath").toString();
            list.append(p);
        }
        settings.endArray();
        return list;
    }

signals:
    void pluginAddRequested(const PluginInfo &plugin);
    void pluginInstallRequested(int pluginId, const QString &pluginName);
    void pluginUninstallRequested(int pluginId, const QString &pluginName);
    void pluginUpdateRequested(int pluginId, const QString &pluginName);
    void pluginRemoveRequested(int pluginId, const QString &pluginName);

protected:
    void paintEvent(QPaintEvent *event) override;

private slots:
    void onAddPluginClicked();

private:
    void initPlugins();
    void setupUI();
    void renderList(const QString &filter = {});
    void selectPlugin(int id);
    void doAction(int id, const QString &action);
    void savePlugins();
    void loadPlugins();

    // data
    QList<PluginInfo> plugins;
    int               selectedId = -1;

    // left panel
    QLineEdit    *searchInput      = nullptr;
    QListWidget  *pluginListWidget = nullptr;

    // right panel
    QWidget      *placeholderWidget   = nullptr;
    QScrollArea  *detailScroll        = nullptr;
    QWidget      *detailPanel         = nullptr;
    QLabel       *detailIcon          = nullptr;
    QLabel       *detailName          = nullptr;
    QLabel       *detailMeta          = nullptr;
    QLabel       *detailStatus        = nullptr;
    QLabel       *detailDesc          = nullptr;
    QWidget      *detailTagsContainer = nullptr;
    QPushButton  *btnInstall          = nullptr;
    QPushButton  *btnUpdate           = nullptr;
    QPushButton  *btnUninstall        = nullptr;
    QPushButton  *btnRemove           = nullptr;
};

#include "pluginmanagerdialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QSplitter>
#include <QFrame>
#include <QScrollArea>
#include <QPainter>
#include <QPainterPath>
#include <QFileDialog>
#include <QFileInfo>
#include <QMessageBox>
#include <algorithm>
#include <QDebug>
#include <QSettings>

PluginManagerDialog::PluginManagerDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle("Plugin Manager");
    setMinimumSize(820, 540);
    resize(880, 560);

setWindowFlags(Qt::Dialog);

    initPlugins();
    setupUI();
    renderList();
}

void PluginManagerDialog::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, false);
    for (int i = 5; i >= 1; --i) {
        QPen glowPen(QColor(0, 191, 255, 18 * i));
        glowPen.setWidth(i * 1);
        painter.setPen(glowPen);
        painter.setBrush(Qt::NoBrush);
        painter.drawRect(rect().adjusted(i, i, -i, -i));
    }
    painter.setPen(QPen(QColor("#27446d"), 1));
    painter.drawRect(rect().adjusted(1, 1, -1, -1));
}

void PluginManagerDialog::initPlugins()
{
    loadPlugins();

    if (plugins.isEmpty()) {
        plugins = {
                   {1, "DIS Protocol Bridge",        "2.4.1", "O2I Core Team",
                    "Enables IEEE 1278.1 DIS packet transmission and reception for distributed simulation "
                    "environments. Supports all major exercise modes including live, virtual, and constructive.",
                    {"DIS","UDP","NATO"},          "installed", "#185FA5"},

                   {2, "AESA Waveform Designer",     "1.1.0", "Sensor Labs",
                    "Advanced waveform design toolkit for AESA radar simulation with pulse compression and "
                    "adaptive beamforming support for complex threat environments.",
                    {"AESA","Radar","Waveform"},   "update",    "#0F6E56"},

                   {3, "Terrain Elevation Engine",   "3.0.2", "GeoSim Corp",
                    "High-resolution DTED terrain data integration for LOS masking, path loss calculation "
                    "and terrain-following simulations across varied geographic regions.",
                    {"DTED","Terrain","LOS"},      "installed", "#3B6D11"},

                   {4, "Tactical Threat Library",    "1.0.4", "DefSec Analytics",
                    "Comprehensive threat platform database with emission signatures, kinematic profiles, "
                    "and engagement envelopes for EW and threat assessment scenarios.",
                    {"Threat","Database","EW"},    "installed", "#A32D2D"},

                   {5, "Multi-Screen HMI Bridge",    "0.9.3", "UX Forge",
                    "Allows sensor display panels to be rendered across multiple external screens with "
                    "real-time sync and configurable layout management.",
                    {"HMI","Multi-display"},       "none",      "#534AB7"},

                   {6, "Link 16 Decoder",            "2.1.0", "CommsLab",
                    "Real-time Link 16 TADIL-J message decoding and display with full J-series message "
                    "support, track correlation and tactical picture compilation.",
                    {"Link16","TADIL","Datalink"}, "none",      "#854F0B"},
                   };
        savePlugins();
    }
}
void PluginManagerDialog::setupUI()
{
    setStyleSheet(R"(
        QDialog { background-color: transparent; color: white; }
        QLineEdit {
            background-color: #1A3A4F;
            border: 2px solid #27446d;
            border-radius: 5px;
            color: white;
            padding: 6px 10px;
            font-size: 12px;
        }
        QLineEdit:focus { border-color: #00BFFF; }
        QListWidget {
            background-color: #0A1E2E;
            border: none;
            color: #aac;
            font-size: 13px;
            outline: none;
        }
        QListWidget::item {
            padding: 9px 12px;
            border-left: 2px solid transparent;
            border-bottom: 0.5px solid #1A3A4F;
        }
        QListWidget::item:hover { background-color: #1A3A4F; color: white; }
        QListWidget::item:selected {
            background-color: #1A3A4F;
            color: #00BFFF;
            border-left: 2px solid #00BFFF;
        }
        QScrollArea { background: transparent; border: none; }
        QLabel#sectionLabel { color: #6b93a8; font-size: 10px; letter-spacing: 1px; }
        QLabel#pluginName   { color: white; font-size: 17px; font-weight: bold; }
        QLabel#pluginMeta   { color: #8aabbb; font-size: 12px; }
        QLabel#pluginDesc   { color: #aabbc8; font-size: 13px; }
        QLabel#statusBadge  { border-radius: 10px; padding: 3px 12px; font-size: 11px; font-weight: bold; }
    )");

    QVBoxLayout *outerLayout = new QVBoxLayout(this);
    outerLayout->setContentsMargins(2, 2, 2, 2);
    outerLayout->setSpacing(0);

    QWidget *container = new QWidget;
    container->setStyleSheet("QWidget { background-color: #0F2636; }");
    outerLayout->addWidget(container);

    QVBoxLayout *mainLayout = new QVBoxLayout(container);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    // ── Topbar ────────────────────────────────────────────────────────────
    QWidget *topbar = new QWidget(container);
    topbar->setObjectName("topbar");
    topbar->setFixedHeight(46);
    topbar->setStyleSheet(
        "QWidget#topbar { background-color: #071820; border-bottom: 1px solid #1A3A4F; }");

    QHBoxLayout *topLayout = new QHBoxLayout(topbar);
    topLayout->setContentsMargins(14, 8, 14, 8);
    topLayout->setAlignment(Qt::AlignVCenter);

    QLabel *titleLbl = new QLabel("  Plugin Manager");
    titleLbl->setStyleSheet("color: white; font-size: 14px; font-weight: bold;");
    topLayout->addWidget(titleLbl);
    topLayout->addStretch();

    QPushButton *addBtn = new QPushButton("+ Add Plugin");
    addBtn->setObjectName("btnAdd");
    addBtn->setFixedHeight(30);
    addBtn->setFixedWidth(110);
    addBtn->setStyleSheet(
        "QPushButton { background-color: #00BFFF; color: #0F2636; border: none;"
        "  border-radius: 6px; font-size: 13px; font-weight: 600; padding: 6px 14px; }"
        "QPushButton:hover { background-color: #33ccff; }"
        "QPushButton:pressed { background-color: #0099cc; }");
    topLayout->addWidget(addBtn);
    mainLayout->addWidget(topbar);

    // ── Body splitter ─────────────────────────────────────────────────────
    QSplitter *splitter = new QSplitter(Qt::Horizontal);
    splitter->setStyleSheet("QSplitter::handle { background: #1A3A4F; width: 1px; }");
    splitter->setHandleWidth(1);

    // ── Left panel ────────────────────────────────────────────────────────
    QWidget *leftWidget = new QWidget;
    leftWidget->setStyleSheet("background-color: #0A1E2E;");
    leftWidget->setMinimumWidth(200);
    leftWidget->setMaximumWidth(260);
    QVBoxLayout *leftLayout = new QVBoxLayout(leftWidget);
    leftLayout->setContentsMargins(0, 0, 0, 0);
    leftLayout->setSpacing(0);

    QWidget *searchContainer = new QWidget;
    searchContainer->setStyleSheet("background: #0A1E2E; border-bottom: 1px solid #1A3A4F;");
    QHBoxLayout *sLayout = new QHBoxLayout(searchContainer);
    sLayout->setContentsMargins(10, 8, 10, 8);
    searchInput = new QLineEdit;
    searchInput->setPlaceholderText("Search plugins...");
    sLayout->addWidget(searchInput);
    leftLayout->addWidget(searchContainer);

    QLabel *listLabel = new QLabel("ALL PLUGINS");
    listLabel->setObjectName("sectionLabel");
    listLabel->setContentsMargins(12, 8, 0, 4);
    leftLayout->addWidget(listLabel);

    pluginListWidget = new QListWidget;
    pluginListWidget->setFocusPolicy(Qt::NoFocus);
    leftLayout->addWidget(pluginListWidget);
    splitter->addWidget(leftWidget);

    // ── Right panel ───────────────────────────────────────────────────────
    QWidget *rightContainer = new QWidget;
    rightContainer->setStyleSheet("background-color: #0F2636;");
    QVBoxLayout *rightLayout = new QVBoxLayout(rightContainer);
    rightLayout->setContentsMargins(0, 0, 0, 0);
    rightLayout->setSpacing(0);

    // Placeholder
    placeholderWidget = new QWidget;
    QVBoxLayout *phLayout = new QVBoxLayout(placeholderWidget);
    phLayout->setAlignment(Qt::AlignCenter);
    QLabel *phIcon = new QLabel("⧉");
    phIcon->setAlignment(Qt::AlignCenter);
    phIcon->setStyleSheet("color: #3A5A7F; font-size: 36px;");
    QLabel *phText = new QLabel("Select a plugin to view details");
    phText->setAlignment(Qt::AlignCenter);
    phText->setStyleSheet("color: #4A7A9F; font-size: 13px;");
    phLayout->addWidget(phIcon);
    phLayout->addWidget(phText);
    rightLayout->addWidget(placeholderWidget);

    // ── Detail wrapper ────────────────────────────────────────────────────
    QWidget *detailWrapper = new QWidget;
    detailWrapper->setStyleSheet("background-color: #0F2636;");
    detailWrapper->setVisible(false);
    QVBoxLayout *wrapLayout = new QVBoxLayout(detailWrapper);
    wrapLayout->setContentsMargins(0, 0, 0, 0);
    wrapLayout->setSpacing(0);

    detailScroll = new QScrollArea;
    detailScroll->setWidgetResizable(true);
    detailScroll->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    detailPanel = new QWidget;
    detailPanel->setStyleSheet("background-color: #0F2636;");
    QVBoxLayout *dLayout = new QVBoxLayout(detailPanel);
    dLayout->setContentsMargins(24, 24, 24, 16);
    dLayout->setSpacing(0);

    detailIcon = new QLabel;
    detailIcon->setFixedSize(50, 50);
    detailIcon->setAlignment(Qt::AlignCenter);
    detailIcon->setStyleSheet("border-radius: 10px; font-size: 22px;");
    dLayout->addWidget(detailIcon);
    dLayout->addSpacing(12);

    detailName = new QLabel;
    detailName->setObjectName("pluginName");
    dLayout->addWidget(detailName);
    dLayout->addSpacing(3);

    detailMeta = new QLabel;
    detailMeta->setObjectName("pluginMeta");
    dLayout->addWidget(detailMeta);
    dLayout->addSpacing(10);

    detailStatus = new QLabel;
    detailStatus->setObjectName("statusBadge");
    detailStatus->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    dLayout->addWidget(detailStatus);
    dLayout->addSpacing(14);

    detailDesc = new QLabel;
    detailDesc->setObjectName("pluginDesc");
    detailDesc->setWordWrap(true);
    dLayout->addWidget(detailDesc);
    dLayout->addSpacing(18);

    QLabel *tagsLabel = new QLabel("TAGS");
    tagsLabel->setObjectName("sectionLabel");
    dLayout->addWidget(tagsLabel);
    dLayout->addSpacing(8);

    detailTagsContainer = new QWidget;
    QHBoxLayout *tagsLayout = new QHBoxLayout(detailTagsContainer);
    tagsLayout->setContentsMargins(0, 0, 0, 0);
    tagsLayout->setSpacing(6);
    dLayout->addWidget(detailTagsContainer);
    dLayout->addStretch();

    detailScroll->setWidget(detailPanel);
    wrapLayout->addWidget(detailScroll);

    // ── Footer ────────────────────────────────────────────────────────────
    QWidget *footer = new QWidget;
    footer->setObjectName("pluginFooter");
    footer->setStyleSheet(
        "QWidget#pluginFooter { background-color: #0F2636; border-top: 1px solid #1A3A4F; }");
    QVBoxLayout *footerLayout = new QVBoxLayout(footer);
    footerLayout->setContentsMargins(24, 12, 24, 16);
    footerLayout->setSpacing(8);

    QWidget *btnRow = new QWidget;
    btnRow->setStyleSheet("background: transparent;");
    QHBoxLayout *btnRowLayout = new QHBoxLayout(btnRow);
    btnRowLayout->setContentsMargins(0, 0, 0, 0);
    btnRowLayout->setSpacing(8);
    btnRowLayout->addStretch();

    btnInstall = new QPushButton("Install");
    btnInstall->setObjectName("btnInstall");
    btnInstall->setVisible(false);
    btnInstall->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    btnInstall->setStyleSheet(
        "QPushButton { background-color: #00BFFF; color: #0F2636; border: none;"
        "  border-radius: 6px; font-size: 13px; font-weight: 600; padding: 10px 20px; }"
        "QPushButton:hover { background-color: #33ccff; }"
        "QPushButton:pressed { background-color: #0099cc; }");
    btnRowLayout->addWidget(btnInstall);

    btnUpdate = new QPushButton("Update to Latest");
    btnUpdate->setObjectName("btnUpdate");
    btnUpdate->setVisible(false);
    btnUpdate->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    btnUpdate->setStyleSheet(
        "QPushButton { background-color: transparent; color: #d4a020;"
        "  border: 1px solid #d4a020; border-radius: 6px;"
        "  font-size: 13px; font-weight: 500; padding: 10px 20px; }"
        "QPushButton:hover { background-color: #2a2000; }");
    btnRowLayout->addWidget(btnUpdate);

    btnUninstall = new QPushButton("Uninstall");
    btnUninstall->setObjectName("btnUninstall");
    btnUninstall->setVisible(false);
    btnUninstall->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    btnUninstall->setStyleSheet(
        "QPushButton { background-color: transparent; color: #e06060;"
        "  border: 1px solid #e06060; border-radius: 6px;"
        "  font-size: 13px; font-weight: 500; padding: 10px 20px; }"
        "QPushButton:hover { background-color: #3a1a1a; }");
    btnRowLayout->addWidget(btnUninstall);

    btnRemove = new QPushButton("✕  Remove");
    btnRemove->setObjectName("btnRemove");
    btnRemove->setVisible(false);
    btnRemove->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    btnRemove->setStyleSheet(
        "QPushButton { background-color: transparent; color: #666;"
        "  border: 1px solid #444; border-radius: 6px;"
        "  font-size: 12px; font-weight: 500; padding: 10px 20px; }"
        "QPushButton:hover { background-color: #e06060; color: #aaa; border-color: #666; }"
        "QPushButton:pressed { background-color: #2a0a0a; color: #e06060; border-color: #e06060; }");
    btnRowLayout->addWidget(btnRemove);

    footerLayout->addWidget(btnRow);
    wrapLayout->addWidget(footer);

    rightLayout->addWidget(detailWrapper);
    splitter->addWidget(rightContainer);

    splitter->setSizes({220, 620});
    mainLayout->addWidget(splitter);

    // ── Connections ───────────────────────────────────────────────────────
    connect(searchInput, &QLineEdit::textChanged, this, [=](const QString &txt){
        renderList(txt);
    });
    connect(pluginListWidget, &QListWidget::currentRowChanged, this, [=](int row){
        if (row < 0) return;
        QListWidgetItem *item = pluginListWidget->item(row);
        if (item) selectPlugin(item->data(Qt::UserRole).toInt());
    });
    connect(btnInstall,   &QPushButton::clicked, this, [=]{ doAction(selectedId, "install");   });
    connect(btnUninstall, &QPushButton::clicked, this, [=]{ doAction(selectedId, "uninstall"); });
    connect(btnUpdate,    &QPushButton::clicked, this, [=]{ doAction(selectedId, "update");    });
    connect(btnRemove,    &QPushButton::clicked, this, [=]{ doAction(selectedId, "remove");    });
    connect(addBtn, &QPushButton::clicked, this, &PluginManagerDialog::onAddPluginClicked);
}

void PluginManagerDialog::renderList(const QString &filter)
{
    pluginListWidget->clear();
    QString q = filter.toLower();
    for (const PluginInfo &p : plugins) {
        bool match = q.isEmpty()
                     || p.name.toLower().contains(q)
                     || std::any_of(p.tags.begin(), p.tags.end(),
                                    [&](const QString &t){ return t.toLower().contains(q); });
        if (!match) continue;
        QString indicator = (p.status == "installed") ? "  ●"
                            : (p.status == "update")  ? "  ◉"
                                                      : "  ○";
        QListWidgetItem *item = new QListWidgetItem(p.name + indicator);
        item->setData(Qt::UserRole, p.id);
        if      (p.status == "installed") item->setForeground(QColor("#aaccdd"));
        else if (p.status == "update")    item->setForeground(QColor("#d4a020"));
        else                              item->setForeground(QColor("#6b8a9a"));
        pluginListWidget->addItem(item);
    }
}

void PluginManagerDialog::selectPlugin(int id)
{
    selectedId = id;
    const PluginInfo *p = nullptr;
    for (const PluginInfo &pl : plugins) if (pl.id == id) { p = &pl; break; }
    if (!p) return;

    placeholderWidget->setVisible(false);
    detailScroll->parentWidget()->setVisible(true);

    detailIcon->setText("⬡");
    detailIcon->setStyleSheet(
        QString("border-radius:10px; font-size:22px; background:%1; color:white;").arg(p->iconColor));

    detailName->setText(p->name);
    detailMeta->setText(QString("v%1  ·  %2").arg(p->version, p->author));

    if (p->status == "installed") {
        detailStatus->setText("  ✔  Installed  ");
        detailStatus->setStyleSheet(
            "QLabel#statusBadge{background:#0d3d2a;color:#1D9E75;"
            "border-radius:10px;padding:3px 12px;font-size:11px;font-weight:bold;}");
    } else if (p->status == "update") {
        detailStatus->setText("  ↑  Update Available  ");
        detailStatus->setStyleSheet(
            "QLabel#statusBadge{background:#2a1e00;color:#d4a020;"
            "border-radius:10px;padding:3px 12px;font-size:11px;font-weight:bold;}");
    } else {
        detailStatus->setText("  ↓  Not Installed  ");
        detailStatus->setStyleSheet(
            "QLabel#statusBadge{background:#1A3A4F;color:#6b93a8;"
            "border-radius:10px;padding:3px 12px;font-size:11px;font-weight:bold;}");
    }

    detailDesc->setText(p->description);

    QLayout *tagsLayout = detailTagsContainer->layout();
    QLayoutItem *child;
    while ((child = tagsLayout->takeAt(0)) != nullptr) {
        delete child->widget();
        delete child;
    }
    for (const QString &tag : p->tags) {
        QLabel *tl = new QLabel(tag);
        tl->setStyleSheet(
            "background:#1A3A4F;color:#8aabbb;"
            "border-radius:10px;padding:3px 10px;font-size:11px;");
        tagsLayout->addWidget(tl);
    }
    static_cast<QHBoxLayout*>(tagsLayout)->addStretch();

    btnInstall->setVisible(p->status == "none");
    btnUpdate->setVisible(p->status == "update");
    btnUninstall->setVisible(p->status == "installed" || p->status == "update");
    btnRemove->setVisible(true);
}

// ── Actions ──────────────────────────────────────────────────────────────────
void PluginManagerDialog::doAction(int id, const QString &action)
{
    QString pluginName;
    QString pluginFilePath;   // ← add
    for (const PluginInfo &p : plugins) {
        if (p.id == id) {
            pluginName     = p.name;
            pluginFilePath = p.filePath;   // ← path bhi lo
            break;
        }
    }

    if (action == "remove") {
        plugins.erase(std::remove_if(plugins.begin(), plugins.end(),
                                     [id](const PluginInfo &p){ return p.id == id; }),
                      plugins.end());
        selectedId = -1;
        savePlugins();
        renderList(searchInput->text());
        placeholderWidget->setVisible(true);
        if (detailScroll && detailScroll->parentWidget())
            detailScroll->parentWidget()->setVisible(false);
        btnInstall->setVisible(false);
        btnUpdate->setVisible(false);
        btnUninstall->setVisible(false);
        btnRemove->setVisible(false);

        qDebug() << "[PluginManager] Signal EMIT → pluginRemoveRequested | name:" << pluginName;
        emit pluginRemoveRequested(id, pluginFilePath);   // ← path bhejo
        return;
    }

    for (PluginInfo &p : plugins) {
        if (p.id != id) continue;
        if (action == "install") {
            p.status = "installed";
            qDebug() << "[PluginManager] Signal EMIT → pluginInstallRequested | name:"
                     << pluginName << "| path:" << pluginFilePath;
            emit pluginInstallRequested(id, pluginFilePath);   // ← path bhejo
        } else if (action == "update") {
            p.status = "installed";
            qDebug() << "[PluginManager] Signal EMIT → pluginUpdateRequested | name:"
                     << pluginName << "| path:" << pluginFilePath;
            emit pluginUpdateRequested(id, pluginFilePath);    // ← path bhejo
        } else if (action == "uninstall") {
            p.status = "none";
            qDebug() << "[PluginManager] Signal EMIT → pluginUninstallRequested | name:"
                     << pluginName << "| path:" << pluginFilePath;
            emit pluginUninstallRequested(id, pluginFilePath); // ← path bhejo
        }
        break;
    }
    savePlugins();
    renderList(searchInput->text());
    selectPlugin(id);
}void PluginManagerDialog::onAddPluginClicked()
{
    QString filePath = QFileDialog::getOpenFileName(
        this,
        "Select Plugin File",
        QString(),
        "Plugin Files (*.plugin *.zip *.tar.gz *.so *.dll);;All Files (*)"
        );

    if (filePath.isEmpty()) return;

    QFileInfo fileInfo(filePath);
    QString pluginName = fileInfo.baseName();

    for (const PluginInfo &p : plugins) {
        if (p.name.toLower() == pluginName.toLower()) {
            QMessageBox::warning(this, "Duplicate Plugin",
                                 QString("'%1' already exists.").arg(pluginName));
            return;
        }
    }

    int newId = 1;
    for (const PluginInfo &p : plugins)
        if (p.id >= newId) newId = p.id + 1;

    static const QStringList colors = {
        "#185FA5", "#0F6E56", "#3B6D11", "#A32D2D",
        "#534AB7", "#854F0B", "#993556", "#085041", "#0C447C"
    };

    PluginInfo newPlugin;
    newPlugin.id          = newId;
    newPlugin.name        = pluginName;
    newPlugin.version     = "1.0.0";
    newPlugin.author      = "Unknown";
    newPlugin.description = QString("Loaded from: %1").arg(filePath);
    newPlugin.filePath    = filePath;   // ← actual path store karo
    newPlugin.tags        = QStringList() << fileInfo.suffix().toUpper();
    newPlugin.status      = "none";
    newPlugin.iconColor   = colors[(newId - 1) % colors.size()];

    plugins.append(newPlugin);
    savePlugins();
    renderList(searchInput->text());
    selectPlugin(newId);

    for (int i = 0; i < pluginListWidget->count(); ++i) {
        if (pluginListWidget->item(i)->data(Qt::UserRole).toInt() == newId) {
            pluginListWidget->setCurrentRow(i);
            break;
        }
    }

    emit pluginAddRequested(newPlugin);
    qDebug() << "[PluginManager] Signal EMIT → pluginAddRequested | name:"
             << newPlugin.name << "| path:" << newPlugin.filePath;
}
void PluginManagerDialog::savePlugins()
{
    QSettings settings("MyApp", "PluginManager");
    settings.beginWriteArray("plugins");
    for (int i = 0; i < plugins.size(); ++i) {
        settings.setArrayIndex(i);
        const PluginInfo &p = plugins[i];
        settings.setValue("id",          p.id);
        settings.setValue("name",        p.name);
        settings.setValue("version",     p.version);
        settings.setValue("author",      p.author);
        settings.setValue("description", p.description);
        settings.setValue("tags",        p.tags);
        settings.setValue("status",      p.status);
        settings.setValue("iconColor",   p.iconColor);
        settings.setValue("filePath",    p.filePath);
    }
    settings.endArray();
    qDebug() << "[PluginManager] Plugins saved. Count:" << plugins.size();
}
void PluginManagerDialog::loadPlugins()
{
    QSettings settings("MyApp", "PluginManager");
    int size = settings.beginReadArray("plugins");
    plugins.clear();
    for (int i = 0; i < size; ++i) {
        settings.setArrayIndex(i);
        PluginInfo p;
        p.id          = settings.value("id").toInt();
        p.name        = settings.value("name").toString();
        p.version     = settings.value("version").toString();
        p.author      = settings.value("author").toString();
        p.description = settings.value("description").toString();
        p.tags        = settings.value("tags").toStringList();
        p.status      = settings.value("status").toString();
        p.iconColor   = settings.value("iconColor").toString();
        p.filePath    = settings.value("filePath").toString();

        if (p.filePath.isEmpty() && p.description.startsWith("Loaded from: ")) {
            p.filePath = p.description;
            p.filePath.remove("Loaded from: ");
        }

        plugins.append(p);
    }
    settings.endArray();
}

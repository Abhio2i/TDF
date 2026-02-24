#ifndef CUSTOMTITLEBAR_H
#define CUSTOMTITLEBAR_H

#include <QWidget>
#include <QLabel>
#include <QPushButton>
#include <QHBoxLayout>
#include <QMouseEvent>

class CustomTitleBar : public QWidget
{
    Q_OBJECT

public:
    explicit CustomTitleBar(QWidget *parentWindow, QWidget *parent = nullptr)
        : QWidget(parent), m_parentWindow(parentWindow), m_dragging(false)
    {
        setFixedHeight(30);
        setObjectName("titleBar");
        setStyleSheet(R"(
            QWidget#titleBar {
                background-color: #0A1A27;
                border-bottom: 1px solid #27446d;
            }
        )");

        QHBoxLayout *layout = new QHBoxLayout(this);
        layout->setContentsMargins(10, 0, 0, 0);
        layout->setSpacing(0);

        m_titleLabel = new QLabel("Indigenous Scenario and Sensor Simulation Toolkit", this);
        m_titleLabel->setStyleSheet(R"(
            QLabel {
                color: #aaaaaa;
                font-size: 11px;
                background-color: transparent;
                padding-left: 8px;
            }
        )");
        layout->addWidget(m_titleLabel);
        layout->addStretch();

        QString btnStyle = R"(
            QPushButton {
                background-color: transparent;
                color: #aaaaaa;
                border: none;
                font-size: 14px;
                padding: 0px;
            }
            QPushButton:hover { background-color: #27446d; color: white; }
            QPushButton:pressed { background-color: #1A3652; }
        )";

        m_minBtn = new QPushButton("─", this);
        m_minBtn->setFixedSize(45, 30);
        m_minBtn->setStyleSheet(btnStyle);
        m_minBtn->setCursor(Qt::ArrowCursor);

        m_maxBtn = new QPushButton("□", this);
        m_maxBtn->setFixedSize(45, 30);
        m_maxBtn->setStyleSheet(btnStyle);
        m_maxBtn->setCursor(Qt::ArrowCursor);

        m_closeBtn = new QPushButton("✕", this);
        m_closeBtn->setFixedSize(45, 30);
        m_closeBtn->setStyleSheet(R"(
            QPushButton {
                background-color: transparent;
                color: #aaaaaa;
                border: none;
                font-size: 13px;
                padding: 0px;
            }
            QPushButton:hover { background-color: #cc0000; color: white; }
            QPushButton:pressed { background-color: #aa0000; }
        )");
        m_closeBtn->setCursor(Qt::ArrowCursor);

        layout->addWidget(m_minBtn);
        layout->addWidget(m_maxBtn);
        layout->addWidget(m_closeBtn);

        connect(m_minBtn,   &QPushButton::clicked, m_parentWindow, &QWidget::showMinimized);
        connect(m_maxBtn,   &QPushButton::clicked, this, &CustomTitleBar::toggleMaximize);
        connect(m_closeBtn, &QPushButton::clicked, m_parentWindow, &QWidget::close);
    }

    void setTitle(const QString &title) {
        m_titleLabel->setText(title);
    }

    QPushButton* maxBtn() { return m_maxBtn; }

protected:
    // ── Drag: press ──────────────────────────────────────────────────────
    void mousePressEvent(QMouseEvent *event) override
    {
        if (event->button() == Qt::LeftButton) {
            m_dragging = true;
            m_dragPos  = event->globalPos() - m_parentWindow->frameGeometry().topLeft();
            event->accept();
        }
    }

    // ── Drag: move ───────────────────────────────────────────────────────
    void mouseMoveEvent(QMouseEvent *event) override
    {
        if (m_dragging && (event->buttons() & Qt::LeftButton)) {
            if (!m_parentWindow->isMaximized()) {
                m_parentWindow->move(event->globalPos() - m_dragPos);
            }
            event->accept();
        }
    }

    // ── Drag: release ────────────────────────────────────────────────────
    void mouseReleaseEvent(QMouseEvent *event) override
    {
        if (event->button() == Qt::LeftButton) {
            m_dragging = false;
            event->accept();
        }
    }

    // ── Double click: maximize/restore ───────────────────────────────────
    void mouseDoubleClickEvent(QMouseEvent *event) override
    {
        if (event->button() == Qt::LeftButton) {
            toggleMaximize();
            event->accept();
        }
    }

private slots:
    void toggleMaximize()
    {
        if (m_parentWindow->isMaximized()) {
            m_parentWindow->showNormal();
            m_maxBtn->setText("□");
        } else {
            m_parentWindow->showMaximized();
            m_maxBtn->setText("❐");
        }
    }

private:
    QWidget     *m_parentWindow;
    QLabel      *m_titleLabel;
    QPushButton *m_minBtn;
    QPushButton *m_maxBtn;
    QPushButton *m_closeBtn;

    bool   m_dragging;
    QPoint m_dragPos;
};

#endif // CUSTOMTITLEBAR_H

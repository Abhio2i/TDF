#include "attitudeindicator.h"

#include <QtMath>
#include <QVBoxLayout>
#include <QSurfaceFormat>
#include <QDebug>

namespace {
const int kPanelW = 190;
const float kTau = 6.2831853f;

const QVector3D kLight = QVector3D(0.5f, 1.0f, 0.3f).normalized();
const QVector3D kCamEye(-17.0f, 9.0f, 15.0f);
const QVector3D kCamTarget(0.0f, 2.5f, 0.0f);
const QVector3D kSky(169 / 255.0f, 205 / 255.0f, 232 / 255.0f);
const QVector3D kGrass1(107 / 255.0f, 143 / 255.0f, 78 / 255.0f);
const QVector3D kGrass2(121 / 255.0f, 154 / 255.0f, 90 / 255.0f);
const QVector3D kRoad(96 / 255.0f, 100 / 255.0f, 106 / 255.0f);

// Aircraft ko thoda bada dikhane ke liye scale
const float kModelScale = 1.3f;

QVector3D ringPt(const QVector<QPointF> &prof, int i, float th, float yc, float zc)
{
    return QVector3D(float(prof[i].x()),
                     yc + float(prof[i].y()) * qCos(th),
                     zc + float(prof[i].y()) * qSin(th));
}

const char *kVertSrc =
    "attribute vec3 a_pos;\n"
    "attribute vec3 a_col;\n"
    "uniform mat4 u_mvp;\n"
    "varying vec3 v_col;\n"
    "void main() {\n"
    "    gl_Position = u_mvp * vec4(a_pos, 1.0);\n"
    "    v_col = a_col;\n"
    "}\n";

const char *kFragSrc =
    "varying vec3 v_col;\n"
    "void main() {\n"
    "    gl_FragColor = vec4(v_col, 1.0);\n"
    "}\n";
}

AttitudeIndicator::AttitudeIndicator(QWidget *parent)
    : QOpenGLWidget(parent),
    pitch(0.0f), roll(0.0f), speed(250.0f), altitude(2500.0f), heading(135.0f)
{
    // OpenGL 2.1 compatibility profile: shader isi ke saath chalta hai
    QSurfaceFormat fmt;
    fmt.setVersion(2, 1);
    fmt.setProfile(QSurfaceFormat::CompatibilityProfile);
    fmt.setDepthBufferSize(24);
    setFormat(fmt);

    setWindowTitle("Advanced Primary Flight Display (PFD)");
    resize(760, 500);
    setFocusPolicy(Qt::StrongFocus);

    Qt::WindowFlags flags = windowFlags();
    flags |= Qt::WindowStaysOnTopHint;
    flags &= ~Qt::WindowCloseButtonHint;
    flags |= Qt::CustomizeWindowHint;
    setWindowFlags(flags);

    buildAircraft();

    m_panel = new QWidget(this);
    m_panel->setStyleSheet("background-color: #1c1c1e;");
    QVBoxLayout *lay = new QVBoxLayout(m_panel);
    lay->setContentsMargins(14, 14, 14, 14);

    auto addRow = [&](const QString &name, QLabel *&valueLbl) {
        QLabel *cap = new QLabel(name, m_panel);
        cap->setStyleSheet("color: #9a9a9a; font-size: 13px;");
        valueLbl = new QLabel("-", m_panel);
        valueLbl->setStyleSheet("color: #ffffff; font-size: 18px; font-weight: bold;");
        lay->addWidget(cap);
        lay->addWidget(valueLbl);
        lay->addSpacing(10);
    };
    addRow("Pitch", m_pitchLbl);
    addRow("Roll", m_rollLbl);
    addRow("Heading", m_headLbl);
    addRow("Speed", m_speedLbl);
    addRow("Altitude", m_altLbl);
    addRow("Vertical speed", m_vsLbl);
    lay->addStretch();
}

AttitudeIndicator::~AttitudeIndicator()
{
    makeCurrent();
    m_vbo.destroy();
    delete m_prog;
    m_prog = nullptr;
    doneCurrent();
}

void AttitudeIndicator::addTri(QVector<float> &v, const QVector3D &a, const QVector3D &b,
                               const QVector3D &c, const QVector3D &col)
{
    const QVector3D pts[3] = {a, b, c};
    for (const QVector3D &p : pts) {
        v << p.x() << p.y() << p.z() << col.x() << col.y() << col.z();
    }
}

// Aircraft model, body frame: x = nose (aage), y = upar, z = right wing
void AttitudeIndicator::buildAircraft()
{
    const QVector3D white(0.93f, 0.94f, 0.95f);
    const QVector3D grey(0.75f, 0.78f, 0.82f);
    const QVector3D dark(0.47f, 0.49f, 0.53f);
    const QVector3D red(0.78f, 0.27f, 0.24f);

    // Fuselage
    const QVector<QPointF> fuse = {
        {-5.5, 0.25}, {-4.6, 0.55}, {4.2, 0.62}, {5.6, 0.5}, {7.2, 0.22}, {8.2, 0.03}
    };
    const int nSides = 14;
    for (int i = 0; i < fuse.size() - 1; ++i) {
        for (int k = 0; k < nSides; ++k) {
            float t0 = kTau * k / nSides;
            float t1 = kTau * (k + 1) / nSides;
            Face f;
            f.pts = { ringPt(fuse, i, t0, 0, 0), ringPt(fuse, i + 1, t0, 0, 0),
                     ringPt(fuse, i + 1, t1, 0, 0), ringPt(fuse, i, t1, 0, 0) };
            f.col = white;
            m_faces.append(f);
        }
    }

    auto mirrorZ = [](const QVector<QVector3D> &pts) {
        QVector<QVector3D> out;
        for (const QVector3D &p : pts) out.append(QVector3D(p.x(), p.y(), -p.z()));
        return out;
    };

    // Main wings
    const QVector<QVector3D> wingR = {
        {1.2f, 0.0f, 0.5f}, {-0.9f, 0.0f, 0.5f}, {-0.4f, 0.0f, 6.2f}, {0.9f, 0.0f, 6.2f}
    };
    m_faces.append({wingR, grey});
    m_faces.append({mirrorZ(wingR), grey});

    // Horizontal tail
    const QVector<QVector3D> tailR = {
        {-5.2f, 0.1f, 0.25f}, {-6.9f, 0.1f, 0.25f}, {-6.6f, 0.1f, 2.4f}, {-5.4f, 0.1f, 2.4f}
    };
    m_faces.append({tailR, grey});
    m_faces.append({mirrorZ(tailR), grey});

    // Vertical fin
    m_faces.append({ {{-5.0f, 0.3f, 0.0f}, {-6.9f, 0.3f, 0.0f},
                     {-6.5f, 2.7f, 0.0f}, {-5.7f, 2.7f, 0.0f}}, red });

    // Engines
    const QVector<QPointF> eprof = { {-0.9, 0.38}, {1.2, 0.36}, {1.6, 0.2} };
    const int eSides = 8;
    for (float zc : {-2.8f, 2.8f}) {
        for (int i = 0; i < eprof.size() - 1; ++i) {
            for (int k = 0; k < eSides; ++k) {
                float t0 = kTau * k / eSides;
                float t1 = kTau * (k + 1) / eSides;
                Face f;
                f.pts = { ringPt(eprof, i, t0, -0.5f, zc), ringPt(eprof, i + 1, t0, -0.5f, zc),
                         ringPt(eprof, i + 1, t1, -0.5f, zc), ringPt(eprof, i, t1, -0.5f, zc) };
                f.col = dark;
                m_faces.append(f);
            }
        }
    }
}

void AttitudeIndicator::initializeGL()
{
    initializeOpenGLFunctions();

    qDebug() << "GL version:" << reinterpret_cast<const char *>(glGetString(GL_VERSION));
    qDebug() << "GL renderer:" << reinterpret_cast<const char *>(glGetString(GL_RENDERER));

    glEnable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);

    m_prog = new QOpenGLShaderProgram(this);
    if (!m_prog->addShaderFromSourceCode(QOpenGLShader::Vertex, kVertSrc))
        qWarning() << "Vertex shader error:" << m_prog->log();
    if (!m_prog->addShaderFromSourceCode(QOpenGLShader::Fragment, kFragSrc))
        qWarning() << "Fragment shader error:" << m_prog->log();
    if (!m_prog->link())
        qWarning() << "Link error:" << m_prog->log();

    m_vbo.create();
}

void AttitudeIndicator::resizeGL(int w, int h)
{
    Q_UNUSED(w);
    Q_UNUSED(h);
    m_panel->setGeometry(width() - kPanelW, 0, kPanelW, height());
}

void AttitudeIndicator::paintGL()
{
    if (!m_prog || !m_prog->isLinked()) {
        // Shader fail hua to kuch draw nahi hoga, sirf sky dikhegi
        glClearColor(kSky.x(), kSky.y(), kSky.z(), 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        updatePanel();
        return;
    }

    const int glW = qMax(1, width() - kPanelW);
    const qreal dpr = devicePixelRatioF();
    glViewport(0, 0, int(glW * dpr), int(height() * dpr));

    glEnable(GL_DEPTH_TEST);
    glClearColor(kSky.x(), kSky.y(), kSky.z(), 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    QMatrix4x4 proj;
    proj.perspective(45.0f, float(glW) / float(qMax(1, height())), 0.5f, 500.0f);
    QMatrix4x4 view;
    view.lookAt(kCamEye, kCamTarget, QVector3D(0.0f, 1.0f, 0.0f));
    const QMatrix4x4 vp = proj * view;

    // Attitude: heading (yaw), pitch, roll
    QMatrix4x4 rot;
    rot.rotate(-heading, 0.0f, 1.0f, 0.0f);
    rot.rotate(pitch, 0.0f, 0.0f, 1.0f);
    rot.rotate(roll, 1.0f, 0.0f, 0.0f);

    const float hoverY = 0.8f + altitude / 1000.0f * 0.9f;
    QMatrix4x4 model;
    model.translate(0.0f, hoverY, 0.0f);
    model *= rot;
    model.scale(kModelScale);

    QVector<float> verts;
    verts.reserve(60000);

    // Ground grid, door jaake fog se sky mein mil jaata hai
    for (int x = -10; x <= 150; x += 10) {
        for (int z = -150; z <= 150; z += 10) {
            QVector3D base;
            if (x >= 0 && x <= 120 && z >= -2 && z < 2) {
                base = kRoad;
            } else {
                int parity = ((int(qRound((x + z) / 10.0)) % 2) + 2) % 2;
                base = parity ? kGrass1 : kGrass2;
            }

            QVector3D c0(float(x), 0.0f, float(z));
            QVector3D c1(float(x + 10), 0.0f, float(z));
            QVector3D c2(float(x + 10), 0.0f, float(z + 10));
            QVector3D c3(float(x), 0.0f, float(z + 10));

            QVector3D center(float(x + 5), 0.0f, float(z + 5));
            float depth = -view.map(center).z();
            float fog = qBound(0.0f, (depth - 25.0f) / 170.0f, 1.0f) * 0.9f;
            QVector3D col = base * (1.0f - fog) + kSky * fog;

            addTri(verts, c0, c1, c2, col);
            addTri(verts, c0, c2, c3, col);
        }
    }

    // Aircraft, flat shading (face normal se brightness)
    for (const Face &f : m_faces) {
        QVector<QVector3D> w;
        w.reserve(f.pts.size());
        for (const QVector3D &p : f.pts) w.append(model.map(p));

        QVector3D n = QVector3D::crossProduct(w[1] - w[0], w[2] - w[0]).normalized();
        float shade = 0.4f + 0.6f * qAbs(QVector3D::dotProduct(n, kLight));
        QVector3D col = f.col * shade;

        addTri(verts, w[0], w[1], w[2], col);
        if (w.size() == 4) addTri(verts, w[0], w[2], w[3], col);
    }

    m_prog->bind();
    m_prog->setUniformValue("u_mvp", vp);

    m_vbo.bind();
    m_vbo.allocate(verts.constData(), verts.size() * int(sizeof(float)));

    const int posLoc = m_prog->attributeLocation("a_pos");
    const int colLoc = m_prog->attributeLocation("a_col");
    const int stride = 6 * int(sizeof(float));

    m_prog->enableAttributeArray(posLoc);
    m_prog->enableAttributeArray(colLoc);
    m_prog->setAttributeBuffer(posLoc, GL_FLOAT, 0, 3, stride);
    m_prog->setAttributeBuffer(colLoc, GL_FLOAT, 3 * int(sizeof(float)), 3, stride);

    glDrawArrays(GL_TRIANGLES, 0, verts.size() / 6);

    m_prog->disableAttributeArray(posLoc);
    m_prog->disableAttributeArray(colLoc);
    m_vbo.release();
    m_prog->release();

    GLenum err = glGetError();
    if (err != GL_NO_ERROR) qWarning() << "GL error:" << Qt::hex << err;

    updatePanel();
}

void AttitudeIndicator::updatePanel()
{
    const float vs = qSin(qDegreesToRadians(pitch)) * speed * 101.3f;

    m_pitchLbl->setText(QString("%1°").arg(qRound(pitch)));
    m_rollLbl->setText(QString("%1°").arg(qRound(roll)));
    m_headLbl->setText(QString("%1°").arg(qRound(heading), 3, 10, QChar('0')));
    m_speedLbl->setText(QString("%1 kt").arg(qRound(speed)));
    m_altLbl->setText(QString("%1 ft").arg(qRound(altitude)));
    m_vsLbl->setText(QString("%1%2 fpm")
                         .arg(vs >= 0 ? "+" : "")
                         .arg(qRound(vs / 10.0f) * 10));
}

/* ========================================================================= */
/* File: scene3dwidget.cpp                                                */
/* Purpose: Implements 3D scene widget with camera and entity management    */
/* ========================================================================= */

#include "scene3dwidget.h"                         // For 3D scene widget class
#include "qcheckbox.h"                             // For checkbox widget
#include "qlabel.h"                                // For label widget
#include "qmath.h"                                 // For math functions
#include "qpushbutton.h"                           // For push button widget
#include "qslider.h"                               // For slider widget
#include "qtimer.h"                                // For timer functionality
#include <Qt3DRender/QCamera>                      // For 3D camera
#include <Qt3DExtras/QOrbitCameraController>       // For orbit camera controller
#include <Qt3DExtras/QPhongMaterial>               // For Phong material
#include <Qt3DExtras/QPlaneMesh>                   // For plane mesh
#include <Qt3DRender/QDirectionalLight>            // For directional light
#include <Qt3DExtras/QCuboidMesh>                  // For cuboid mesh
#include <Qt3DExtras/QForwardRenderer>             // For forward renderer
#include <Qt3DExtras/QDiffuseMapMaterial>          // For diffuse map material
#include <Qt3DRender/QTexture>                     // For texture handling
#include <Qt3DRender/QTextureImage>                // For texture images
#include <Qt3DRender/QRenderStateSet>              // For render state set
#include <Qt3DRender/QLineWidth>                   // For line width control
#include <Qt3DExtras/QCuboidGeometry>              // For cuboid geometry
#include <Qt3DExtras/QPlaneGeometry>               // For plane geometry
#include <Qt3DRender/QObjectPicker>                // For object picking
#include <Qt3DRender/QPickEvent>                   // For pick events
#include <cmath>                                   // For standard math
#include <qcolordialog.h>                          // For color dialog
#include <Qt3DRender/QMesh>                        // For mesh loading
#include <Qt3DCore/QTransform>                     // For transform component
#include <QComboBox>                               // For combo box widget
#include <QHBoxLayout>                             // For horizontal layout

// %%% Constructor %%%
/* Initialize 3D scene widget */
Scene3DWidget::Scene3DWidget(QWidget *parent)
    : QWidget(parent)
{
    // Create 3D view
    view = new Qt3DExtras::Qt3DWindow();
    // Set up main layout
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    // Create top UI bar
    QHBoxLayout *topBar = new QHBoxLayout();
    auto *colorButton = new QPushButton("Change Background Color");
    auto *modeBox = new QComboBox();
    modeBox->addItem("Full Render");
    modeBox->addItem("Wireframe");
    modeBox->addItem("Wireframe Overlay");
    topBar->addWidget(modeBox);
    auto *followToggle = new QCheckBox("Follow Selected");
    auto *lightLabel = new QLabel("Light Intensity");
    auto *lightSlider = new QSlider(Qt::Horizontal);
    lightSlider->setRange(0, 100);
    lightSlider->setValue(30);
    topBar->addWidget(colorButton);
    topBar->addWidget(followToggle);
    topBar->addWidget(lightLabel);
    topBar->addWidget(lightSlider);
    topBar->addStretch();
    // Create 3D view container
    QWidget *container = QWidget::createWindowContainer(view, this);
    container->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    mainLayout->addLayout(topBar);
    mainLayout->addWidget(container);
    // Initialize scene
    Qt3DCore::QEntity *sceneRoot = createScene();
    view->setRootEntity(sceneRoot);
    view->defaultFrameGraph()->setClearColor(QColor(50, 50, 50));
    // Set up camera update timer
    QTimer *cameraUpdateTimer = new QTimer(this);
    connect(cameraUpdateTimer, &QTimer::timeout, this, &Scene3DWidget::updateCamera);
    cameraUpdateTimer->start(16);
    // Configure container focus and mouse tracking
    container->setFocusPolicy(Qt::StrongFocus);
    container->setFocus();
    container->installEventFilter(this);
    setMouseTracking(true);
    container->setMouseTracking(true);
    // Connect UI signals
    connect(colorButton, &QPushButton::clicked, this, [=]() {
        QColor color = QColorDialog::getColor(Qt::white, this, "Pick Background Color");
        if (color.isValid()) {
            view->defaultFrameGraph()->setClearColor(color);
        }
    });
    connect(followToggle, &QCheckBox::toggled, this, [=](bool checked) {
        followSelectedEntity = checked;
    });
    connect(lightSlider, &QSlider::valueChanged, this, [=](int value) {
        if (mainLight)
            mainLight->setIntensity(value / 10.0f);
    });
    connect(modeBox, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [=](int index) {
        currentRenderMode = static_cast<RenderMode>(index);
        applyRenderMode(groundEntity->componentsOfType<Qt3DRender::QGeometryRenderer>()[0]);
        for (auto& [key, entry] : entities3D) {
            auto renderers = entry.entity->componentsOfType<Qt3DRender::QGeometryRenderer>();
            if (!renderers.isEmpty()) {
                applyRenderMode(renderers[0]);
            }
        }
    });
}

// %%% Event Handling %%%
/* Handle key and mouse events */
bool Scene3DWidget::eventFilter(QObject *watched, QEvent *event)
{
    if (event->type() == QEvent::KeyPress) {
        QKeyEvent *keyEvent = static_cast<QKeyEvent *>(event);
        pressedKeys.insert(keyEvent->key());
        return true;
    }
    if (event->type() == QEvent::KeyRelease) {
        QKeyEvent *keyEvent = static_cast<QKeyEvent *>(event);
        pressedKeys.remove(keyEvent->key());
        return true;
    }
    if (event->type() == QEvent::MouseButtonPress) {
        QMouseEvent *mouseEvent = static_cast<QMouseEvent *>(event);
        if (mouseEvent->button() == Qt::RightButton) {
            rightMousePressed = true;
            lastMousePos = mouseEvent->pos();
        } else if (mouseEvent->button() == Qt::MiddleButton) {
            middleMousePressed = true;
            lastMiddlePos = mouseEvent->pos();
            setCursor(Qt::ClosedHandCursor);
        }
        return true;
    }
    if (event->type() == QEvent::MouseButtonRelease) {
        QMouseEvent *mouseEvent = static_cast<QMouseEvent *>(event);
        if (mouseEvent->button() == Qt::RightButton) {
            rightMousePressed = false;
        } else if (mouseEvent->button() == Qt::MiddleButton) {
            middleMousePressed = false;
            setCursor(Qt::ArrowCursor);
        }
        return true;
    }
    if (event->type() == QEvent::MouseMove) {
        QMouseEvent *mouseEvent = static_cast<QMouseEvent *>(event);
        if (rightMousePressed) {
            // Handle rotation
            QPoint delta = mouseEvent->pos() - lastMousePos;
            lastMousePos = mouseEvent->pos();
            float sensitivity = 0.3f;
            yaw += delta.x() * sensitivity;
            pitch -= delta.y() * sensitivity;
            pitch = std::clamp(pitch, -89.0f, 89.0f);
            updateCameraRotation();
            return true;
        }
        if (middleMousePressed) {
            // Handle panning
            QPoint delta = mouseEvent->pos() - lastMiddlePos;
            lastMiddlePos = mouseEvent->pos();
            Qt3DRender::QCamera *camera = view->camera();
            QVector3D forward = (camera->viewCenter() - camera->position()).normalized();
            QVector3D right = QVector3D::crossProduct(forward, camera->upVector()).normalized();
            QVector3D up = camera->upVector().normalized();
            float panSpeed = 0.05f;
            QVector3D offset = (-right * delta.x() + up * delta.y()) * panSpeed;
            camera->setPosition(camera->position() + offset);
            camera->setViewCenter(camera->viewCenter() + offset);
            return true;
        }
    }
    return QWidget::eventFilter(watched, event);
}

/* Handle mouse press events */
void Scene3DWidget::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::RightButton) {
        rightMousePressed = true;
        lastMousePos = event->pos();
    }
}

/* Handle mouse release events */
void Scene3DWidget::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::RightButton) {
        rightMousePressed = false;
    }
}

/* Handle mouse move events */
void Scene3DWidget::mouseMoveEvent(QMouseEvent *event)
{
    if (!rightMousePressed) return;
    QPoint delta = event->pos() - lastMousePos;
    lastMousePos = event->pos();
    float sensitivity = 0.3f;
    yaw += delta.x() * sensitivity;
    pitch -= delta.y() * sensitivity;
    pitch = std::clamp(pitch, -89.0f, 89.0f);
    updateCameraRotation();
}

/* Handle key press events */
void Scene3DWidget::keyPressEvent(QKeyEvent *event)
{
    pressedKeys.insert(event->key());
    QWidget::keyPressEvent(event);
}

/* Handle key release events */
void Scene3DWidget::keyReleaseEvent(QKeyEvent *event)
{
    pressedKeys.remove(event->key());
    QWidget::keyReleaseEvent(event);
}

// %%% Scene Creation %%%
/* Create 3D scene with ground, light, and model */
Qt3DCore::QEntity *Scene3DWidget::createScene()
{
    rootEntity = new Qt3DCore::QEntity();
    // Configure camera
    Qt3DRender::QCamera *camera = view->camera();
    camera->lens()->setPerspectiveProjection(100.0f, 16.0f/9.0f, 0.1f, 1000.0f);
    camera->setPosition(QVector3D(0, 1, 4));
    camera->setViewCenter(QVector3D(0, 0, 0));
    // Create ground entity
    groundEntity = new Qt3DCore::QEntity(rootEntity);
    auto *planeGeometry = new Qt3DExtras::QPlaneGeometry(groundEntity);
    planeGeometry->setWidth(1000);
    planeGeometry->setHeight(1000);
    auto *renderer = new Qt3DRender::QGeometryRenderer();
    renderer->setGeometry(planeGeometry);
    applyRenderMode(renderer);
    groundEntity->addComponent(renderer);
    // Add ground texture
    auto *texture = new Qt3DRender::QTexture2D();
    texture->setWrapMode(Qt3DRender::QTextureWrapMode(Qt3DRender::QTextureWrapMode::Repeat));
    texture->setMinificationFilter(Qt3DRender::QTexture2D::LinearMipMapLinear);
    texture->setMagnificationFilter(Qt3DRender::QTexture2D::Linear);
    auto *image = new Qt3DRender::QTextureImage();
    image->setSource(QUrl::fromLocalFile(":/texture/images/Texture/grid.png"));
    texture->addTextureImage(image);
    auto *groundMaterial = new Qt3DExtras::QDiffuseMapMaterial();
    groundMaterial->setDiffuse(texture);
    groundMaterial->setShininess(0.0f);
    groundMaterial->setSpecular(Qt::white);
    groundMaterial->setTextureScale(50);
    groundEntity->addComponent(groundMaterial);
    // Add directional light
    auto *lightEntity = new Qt3DCore::QEntity(rootEntity);
    mainLight = new Qt3DRender::QDirectionalLight(lightEntity);
    mainLight->setIntensity(0.7);
    mainLight->setWorldDirection(QVector3D(-1, -1, -1));
    lightEntity->addComponent(mainLight);
    // Create model entity
    auto *modelEntity = new Qt3DCore::QEntity(rootEntity);
    auto *mesh = new Qt3DRender::QMesh(modelEntity);
    mesh->setSource(QUrl::fromLocalFile(":/model/airplane/Model/Airplane/11803_Airplane_v1_l1.obj"));
    modelEntity->addComponent(mesh);
    // Add model texture
    auto *texture2 = new Qt3DRender::QTexture2D();
    auto *image2 = new Qt3DRender::QTextureImage();
    image2->setSource(QUrl::fromLocalFile(":/model/airplane/Model/Airplane/11803_Airplane_body_diff.jpg"));
    texture2->addTextureImage(image2);
    auto *material = new Qt3DExtras::QDiffuseMapMaterial();
    material->setDiffuse(texture2);
    material->setShininess(10.0f);
    material->setSpecular(Qt::white);
    modelEntity->addComponent(material);
    // Apply model transform
    auto *transform = new Qt3DCore::QTransform();
    transform->setScale(0.001f);
    transform->setTranslation(QVector3D(0, 2, 0));
    transform->setRotationX(-90);
    modelEntity->addComponent(transform);
    // Configure view
    view->setFlags(Qt::FramelessWindowHint);
    view->installEventFilter(this);
    return rootEntity;
}

/* Create grid lines for scene */
Qt3DCore::QEntity* Scene3DWidget::createGridLines(Qt3DCore::QEntity *parent, int size, int spacing)
{
    auto *entity = new Qt3DCore::QEntity(parent);
    // Define grid vertices
    QVector<QVector3D> vertices;
    for (int i = -size; i <= size; i += spacing) {
        vertices.append(QVector3D(i, 0.01f, -size));
        vertices.append(QVector3D(i, 0.01f, size));
        vertices.append(QVector3D(-size, 0.01f, i));
        vertices.append(QVector3D(size, 0.01f, i));
    }
    QByteArray vertexData(reinterpret_cast<const char*>(vertices.constData()), vertices.size() * sizeof(QVector3D));
    // Configure renderer
    auto *renderer = new Qt3DRender::QGeometryRenderer();
    renderer->setPrimitiveType(Qt3DRender::QGeometryRenderer::Lines);
    // Configure material
    auto *material = new Qt3DExtras::QPhongMaterial();
    material->setDiffuse(Qt::lightGray);
    entity->addComponent(renderer);
    entity->addComponent(material);
    return entity;
}

// %%% Entity Management %%%
/* Add a 3D entity to the scene */
void Scene3DWidget::addEntity(QString ID, Mesh3dEntry entry)
{
    std::string key = ID.toStdString();
    if (entities3D.find(key) != entities3D.end()) {
        qWarning() << "Entity already exists:" << ID;
        return;
    }
    // Create entity
    auto *entity = new Qt3DCore::QEntity(rootEntity);
    auto *renderer = new Qt3DRender::QGeometryRenderer();
    renderer->setGeometry(new Qt3DExtras::QCuboidGeometry(entity));
    applyRenderMode(renderer);
    entity->addComponent(renderer);
    // Add transform
    entity->addComponent(entry.transform);
    // Add texture material
    QString texturePath = ":/texture/images/Texture/waall.jpg";
    Qt3DRender::QTexture2D *texture = nullptr;
    auto it = textureCache.find(texturePath);
    if (it != textureCache.end()) {
        texture = it->second;
    } else {
        texture = new Qt3DRender::QTexture2D();
        auto *image = new Qt3DRender::QTextureImage();
        image->setSource(QUrl::fromLocalFile(texturePath));
        texture->addTextureImage(image);
        textureCache[texturePath] = texture;
    }
    auto *mat = new Qt3DExtras::QDiffuseMapMaterial();
    if (texture) {
        mat->setDiffuse(texture);
    } else {
        qWarning() << "Texture is null, skipping diffuse assignment.";
    }
    mat->setSpecular(QColor(255, 255, 255));
    mat->setShininess(0.0f);
    mat->setAmbient(QColor(60, 60, 60));
    entity->addComponent(mat);
    // Add object picker
    auto *picker = new Qt3DRender::QObjectPicker(entity);
    picker->setHoverEnabled(true);
    entity->addComponent(picker);
    // Connect picker signal
    connect(picker, &Qt3DRender::QObjectPicker::clicked, this, [=](Qt3DRender::QPickEvent *event) {
        qDebug() << "Clicked Entity ID:" << QString::fromStdString(key);
        emit selectEntityByCursor(ID);
        selectedEntity(QString::fromStdString(key));
    });
    // Store entity
    entry.entity = entity;
    entry.originalMaterial = mat;
    entities3D[key] = entry;
    qDebug() << "Added entity:" << ID;
}

/* Remove a 3D entity from the scene */
void Scene3DWidget::removeEntity(QString ID)
{
    std::string key = ID.toStdString();
    if (entities3D.find(key) == entities3D.end()) {
        qWarning() << "Entity not found:" << ID;
        return;
    }
    Qt3DCore::QEntity *entity = entities3D[key].entity;
    if (entities3D[key].originalMaterial) {
        entity->removeComponent(entities3D[key].originalMaterial);
    }
    delete entity;
    entities3D.erase(key);
    // Clear selection if removed
    if (selectedEntityId == ID) {
        selectedEntityId.clear();
    }
    qDebug() << "Removed entity:" << ID;
}

/* Highlight selected entity */
void Scene3DWidget::selectedEntity(QString ID)
{
    std::string key = ID.toStdString();
    if (entities3D.find(key) == entities3D.end()) return;
    // Restore previous entity's material
    if (!selectedEntityId.isEmpty() && selectedEntityId != ID) {
        std::string prevKey = selectedEntityId.toStdString();
        if (entities3D.find(prevKey) != entities3D.end()) {
            auto &prevEntry = entities3D[prevKey];
            if (prevEntry.entity && prevEntry.originalMaterial) {
                auto comps = prevEntry.entity->componentsOfType<Qt3DRender::QMaterial>();
                for (auto *mat : comps) {
                    if (mat == sharedHighlightMaterial)
                        prevEntry.entity->removeComponent(mat);
                }
                if (!comps.contains(prevEntry.originalMaterial))
                    prevEntry.entity->addComponent(prevEntry.originalMaterial);
            }
        }
    }
    if (selectedEntityId == ID) return;
    selectedEntityId = ID;
    auto &entry = entities3D[key];
    Qt3DCore::QEntity *entity = entry.entity;
    // Store original material
    if (!entry.originalMaterial) {
        auto originals = entity->componentsOfType<Qt3DRender::QMaterial>();
        if (!originals.isEmpty()) entry.originalMaterial = originals[0];
    }
    if (entry.originalMaterial)
        entity->removeComponent(entry.originalMaterial);
    // Apply highlight material
    if (!sharedHighlightMaterial) {
        sharedHighlightMaterial = new Qt3DExtras::QPhongMaterial();
        sharedHighlightMaterial->setAmbient(Qt::yellow);
        sharedHighlightMaterial->setSpecular(Qt::black);
        sharedHighlightMaterial->setShininess(0);
    }
    entity->addComponent(sharedHighlightMaterial);
    qDebug() << "Highlighted with shared material:" << ID;
}

/* Clear texture and material buffers */
void Scene3DWidget::clearBuffer()
{
    delete sharedHighlightMaterial;
    textureCache.clear();
}

/* Update all entities' transforms and materials */
void Scene3DWidget::updateEntities()
{
    for (auto& [key, entry] : entities3D) {
        Qt3DCore::QEntity* entity = entry.entity;
        if (!entity) {
            qWarning() << "Null entity for key:" << QString::fromStdString(key);
            continue;
        }
        // Update transform
        auto transforms = entity->componentsOfType<Qt3DCore::QTransform>();
        if (!transforms.isEmpty()) {
            auto *transform = transforms[0];
            if (transform && entry.position && entry.rotation && entry.size) {
                // Transform updates commented out
            }
        } else {
            qWarning() << "Transform missing for key:" << QString::fromStdString(key);
        }
        // Update material
        auto materials = entity->componentsOfType<Qt3DExtras::QPhongMaterial>();
        if (!materials.isEmpty()) {
            auto *mat = materials[0];
            if (mat && entry.mesh && entry.mesh->color) {
                // Material update commented out
            }
        }
    }
}

/* Move entity to new position */
void Scene3DWidget::MoveEntity(QString ID)
{
    std::string key = ID.toStdString();
    if (entities3D.find(key) == entities3D.end()) {
        qWarning() << "MoveEntity failed. Entity not found:" << ID;
        return;
    }
    Mesh3dEntry &entry = entities3D[key];
    Qt3DCore::QEntity* entity = entry.entity;
    if (!entity) {
        qWarning() << "MoveEntity failed. Entity pointer null:" << ID;
        return;
    }
    auto transforms = entity->componentsOfType<Qt3DCore::QTransform>();
    if (!transforms.isEmpty()) {
        Qt3DCore::QTransform* transform = transforms[0];
        if (transform && entry.position && entry.rotation && entry.size) {
            // Transform updates commented out
            // qDebug() << "Moved entity:" << ID;
        }
    } else {
        qWarning() << "MoveEntity failed. No transform component found for:" << ID;
    }
}

/* Focus camera on entity */
void Scene3DWidget::focusEntity(QString ID)
{
    std::string key = ID.toStdString();
    if (entities3D.find(key) == entities3D.end()) {
        qWarning() << "FocusEntity failed. Entity not found:" << ID;
        return;
    }
    Mesh3dEntry &entry = entities3D[key];
    Qt3DCore::QEntity* entity = entry.entity;
    if (!entity) {
        qWarning() << "FocusEntity failed. Entity pointer null:" << ID;
        return;
    }
    auto transforms = entity->componentsOfType<Qt3DCore::QTransform>();
    if (transforms.isEmpty()) {
        qWarning() << "FocusEntity failed. No transform found for:" << ID;
        return;
    }
    QVector3D entityPos = transforms[0]->translation();
    Qt3DRender::QCamera *camera = view->camera();
    QVector3D offset(0, 3, -5);
    camera->setPosition(entityPos + offset);
    camera->setViewCenter(entityPos);
    QVector3D dir = (entityPos - camera->position()).normalized();
    yaw = qRadiansToDegrees(std::atan2(dir.x(), dir.z()));
    pitch = qRadiansToDegrees(std::asin(dir.y()));
}

/* Update camera position and orientation */
void Scene3DWidget::updateCamera()
{
    if (!view || !view->camera()) return;
    Qt3DRender::QCamera *camera = view->camera();
    // Follow selected entity if enabled
    if (followSelectedEntity && !selectedEntityId.isEmpty()) {
        std::string key = selectedEntityId.toStdString();
        auto it = entities3D.find(key);
        if (it != entities3D.end() && it->second.entity) {
            auto transforms = it->second.entity->componentsOfType<Qt3DCore::QTransform>();
            if (!transforms.isEmpty()) {
                QVector3D entityPos = transforms[0]->translation();
                QVector3D cameraOffset(5, 3, 0);
                camera->setPosition(entityPos + cameraOffset);
                camera->setViewCenter(entityPos);
                return;
            }
        }
    }
    // Manual camera control
    QVector3D forward = (camera->viewCenter() - camera->position()).normalized();
    QVector3D right = QVector3D::crossProduct(forward, camera->upVector()).normalized();
    QVector3D up = camera->upVector().normalized();
    QVector3D position = camera->position();
    float speed = 0.5f;
    if (pressedKeys.contains(Qt::Key_W)) position += forward * speed;
    if (pressedKeys.contains(Qt::Key_S)) position -= forward * speed;
    if (pressedKeys.contains(Qt::Key_A)) position -= right * speed;
    if (pressedKeys.contains(Qt::Key_D)) position += right * speed;
    if (pressedKeys.contains(Qt::Key_E)) position += up * speed;
    if (pressedKeys.contains(Qt::Key_Q)) position -= up * speed;
    if (pressedKeys.contains(Qt::Key_Shift)) speed *= 3.0f;
    camera->setPosition(position);
    camera->setViewCenter(position + forward);
}

/* Update camera rotation based on yaw and pitch */
void Scene3DWidget::updateCameraRotation()
{
    if (!selectedEntityId.isEmpty() && followSelectedEntity) return;
    Qt3DRender::QCamera *camera = view->camera();
    QVector3D direction;
    direction.setX(cos(qDegreesToRadians(yaw)) * cos(qDegreesToRadians(-pitch)));
    direction.setY(sin(qDegreesToRadians(pitch)));
    direction.setZ(sin(qDegreesToRadians(yaw)) * cos(qDegreesToRadians(-pitch)));
    QVector3D newCenter = camera->position() + direction.normalized();
    camera->setViewCenter(newCenter);
}

/* Apply render mode to geometry renderer */
void Scene3DWidget::applyRenderMode(Qt3DRender::QGeometryRenderer* renderer)
{
    switch (currentRenderMode) {
    case RenderMode::Wireframe:
        renderer->setPrimitiveType(Qt3DRender::QGeometryRenderer::Lines);
        break;
    case RenderMode::WireframeWithDraw:
        renderer->setPrimitiveType(Qt3DRender::QGeometryRenderer::LineStrip);
        break;
    case RenderMode::Full:
    default:
        renderer->setPrimitiveType(Qt3DRender::QGeometryRenderer::Triangles);
        break;
    }
}

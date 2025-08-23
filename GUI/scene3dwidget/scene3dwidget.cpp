#include "scene3dwidget.h"
#include "qcheckbox.h"
#include "qlabel.h"
#include "qpushbutton.h"
#include "qslider.h"
#include "qtimer.h"

#include <Qt3DRender/QCamera>
#include <Qt3DExtras/QOrbitCameraController>
// #include <Qt3DCore/QBuffer>
#include <Qt3DCore/QBuffer>
#include <Qt3DCore/QAttribute>
#include <Qt3DExtras/QPhongMaterial>
#include <Qt3DExtras/QPlaneMesh>
#include <Qt3DRender/QDirectionalLight>
#include <Qt3DExtras/QCuboidMesh>
#include <Qt3DExtras/QForwardRenderer>
#include <Qt3DExtras/QDiffuseMapMaterial>
#include <Qt3DRender/QTexture>
#include <Qt3DRender/QTextureImage>
#include <Qt3DRender/QRenderStateSet>
#include <Qt3DRender/QLineWidth>
#include <Qt3DExtras/QCuboidGeometry>
#include <Qt3DExtras/QPlaneGeometry>
#include <Qt3DRender/QObjectPicker>
#include <Qt3DRender/QPickEvent>
#include <qcolordialog.h>
#include <Qt3DRender/QMesh>
#include <Qt3DCore/QTransform>

#include <QComboBox>
#include <QHBoxLayout>

Scene3DWidget::Scene3DWidget(QWidget *parent)
    : QWidget(parent)
{
    view = new Qt3DExtras::Qt3DWindow();
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);

    // 🧁 Top UI bar
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
    lightSlider->setRange(0, 100);     // 0.0 to 10.0 (x10 scaling)
    lightSlider->setValue(30);         // default = 3.0

    topBar->addWidget(colorButton);
    topBar->addWidget(followToggle);
    topBar->addWidget(lightLabel);
    topBar->addWidget(lightSlider);

    topBar->addStretch(); // Push to left

    // // 🖼️ 3D View container
    // QWidget *container = QWidget::createWindowContainer(view, this);
    // container->setMinimumSize(300, 300);
    // In Scene3DWidget constructor
    QWidget *container = QWidget::createWindowContainer(view, this);
    // container->setMinimumSize(100, 100);  // Ensure minimum size
    container->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    mainLayout->addLayout(topBar);
    mainLayout->addWidget(container);

    Qt3DCore::QEntity *sceneRoot = createScene();
    view->setRootEntity(sceneRoot);
    view->defaultFrameGraph()->setClearColor(QColor(50, 50, 50)); // Light Sky Blue
    QTimer *cameraUpdateTimer = new QTimer(this);
    connect(cameraUpdateTimer, &QTimer::timeout, this, &Scene3DWidget::updateCamera);
    cameraUpdateTimer->start(16); // ~60 FPS
    container->setFocusPolicy(Qt::StrongFocus);
    container->setFocus();
    container->installEventFilter(this); // ✅ Catches key events from the actual Qt3D container
    setMouseTracking(true);
    container->setMouseTracking(true);

    connect(colorButton, &QPushButton::clicked, this, [=]() {
        QColor color = QColorDialog::getColor(Qt::white, this, "Pick Background Color");
        if (color.isValid()) {
            view->defaultFrameGraph()->setClearColor(color);
        }
    });

    connect(followToggle, &QCheckBox::toggled, this, [=](bool checked) {
        followSelectedEntity = checked; // Add this flag in your class!
    });

    connect(lightSlider, &QSlider::valueChanged, this, [=](int value) {
        if (mainLight)
            mainLight->setIntensity(value / 10.0f);  // scale 0–10
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
            //setCursor(Qt::CrossCursor);
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
            //setCursor(Qt::ArrowCursor);
        } else if (mouseEvent->button() == Qt::MiddleButton) {
            middleMousePressed = false;
            setCursor(Qt::ArrowCursor);
        }
        return true;
    }

    if (event->type() == QEvent::MouseMove) {
        QMouseEvent *mouseEvent = static_cast<QMouseEvent *>(event);

        // 🎯 Right Button → Rotate (like Unity)
        if (rightMousePressed) {
            QPoint delta = mouseEvent->pos() - lastMousePos;
            lastMousePos = mouseEvent->pos();

            float sensitivity = 0.3f;
            yaw += delta.x() * sensitivity;
            pitch -= delta.y() * sensitivity;
            pitch = std::clamp(pitch, -89.0f, 89.0f);

            updateCameraRotation();
            return true;
        }

        // 🎯 Middle Button → Pan
        if (middleMousePressed) {
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

void Scene3DWidget::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::RightButton) {
        rightMousePressed = true;
        lastMousePos = event->pos();
    }
}

void Scene3DWidget::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::RightButton) {
        rightMousePressed = false;
    }
}

void Scene3DWidget::mouseMoveEvent(QMouseEvent *event)
{
    if (!rightMousePressed) return;

    QPoint delta = event->pos() - lastMousePos;
    lastMousePos = event->pos();

    float sensitivity = 0.3f;
    yaw += delta.x() * sensitivity;
    pitch -= delta.y() * sensitivity;

    // Clamp pitch for safety (to avoid flip)
    pitch = std::clamp(pitch, -89.0f, 89.0f);

    updateCameraRotation(); // 👈 we'll define this
}

void Scene3DWidget::keyPressEvent(QKeyEvent *event)
{
    pressedKeys.insert(event->key());
    QWidget::keyPressEvent(event); // pass to base
}

void Scene3DWidget::keyReleaseEvent(QKeyEvent *event)
{
    pressedKeys.remove(event->key());
    QWidget::keyReleaseEvent(event); // pass to base
}

Qt3DCore::QEntity *Scene3DWidget::createScene()
{
    rootEntity = new Qt3DCore::QEntity();
    // ✅ Grid call here
    //createGridLines(rootEntity, 40, 1);  // 40x40 grid, 1 unit spacing

    // Camera
    Qt3DRender::QCamera *camera = view->camera();
    camera->lens()->setPerspectiveProjection(100.0f, 16.0f / 9.0f, 0.1f, 1000.0f);
    camera->setPosition(QVector3D(0, 1, 4));
    camera->setViewCenter(QVector3D(0, 0, 0));

    // Ground
    groundEntity = new Qt3DCore::QEntity(rootEntity);
    // auto *planeMesh = new Qt3DExtras::QPlaneMesh();
    // planeMesh->setWidth(40);
    // planeMesh->setHeight(40);

    auto *planeGeometry = new Qt3DExtras::QPlaneGeometry(groundEntity);
    planeGeometry->setWidth(1000);
    planeGeometry->setHeight(1000);
    //planeGeometry->setResolution(QSize(1,1));

    auto *renderer = new Qt3DRender::QGeometryRenderer();
    renderer->setGeometry(planeGeometry);
    applyRenderMode(renderer);
    groundEntity->addComponent(renderer);

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
    //groundEntity->addComponent(planeMesh);
    groundEntity->addComponent(groundMaterial);

    // Directional Light
    auto *lightEntity = new Qt3DCore::QEntity(rootEntity);
    mainLight  = new Qt3DRender::QDirectionalLight(lightEntity);
    mainLight ->setIntensity(0.7);
    mainLight ->setWorldDirection(QVector3D(-1, -1, -1));
    lightEntity->addComponent(mainLight );

    // Qt3DExtras::QOrbitCameraController *camController = new Qt3DExtras::QOrbitCameraController(rootEntity);
    // camController->setCamera(view->camera());
    // camController->setLinearSpeed(50.0f);
    // camController->setLookSpeed(180.0f);

    auto *modelEntity = new Qt3DCore::QEntity(rootEntity);

    // 🛠️ Mesh loader
    auto *mesh = new Qt3DRender::QMesh(modelEntity);
    mesh->setSource(QUrl::fromLocalFile(":/model/airplane/Model/Airplane/11803_Airplane_v1_l1.obj")); // .obj file

    modelEntity->addComponent(mesh);

    // 🎨 Material banate hai
    auto *texture2 = new Qt3DRender::QTexture2D();
    auto *image2 = new Qt3DRender::QTextureImage();
    image2->setSource(QUrl::fromLocalFile(":/model/airplane/Model/Airplane/11803_Airplane_body_diff.jpg")); // image path
    texture2->addTextureImage(image2);

    auto *material = new Qt3DExtras::QDiffuseMapMaterial();
    material->setDiffuse(texture2);
    material->setShininess(10.0f); // Thoda shine dena
    material->setSpecular(Qt::white);

    modelEntity->addComponent(material);

    // 🧿 Optional: Transform
    auto *transform = new Qt3DCore::QTransform();
    transform->setScale(0.001f);
    transform->setTranslation(QVector3D(0, 2, 0));
    transform->setRotationX(-90);
    modelEntity->addComponent(transform);


    view->setFlags(Qt::FramelessWindowHint);
    view->installEventFilter(this); // optional

    return rootEntity;
}

Qt3DCore::QEntity* Scene3DWidget::createGridLines(Qt3DCore::QEntity *parent, int size, int spacing)
{
    auto *entity = new Qt3DCore::QEntity(parent);

    QVector<QVector3D> vertices;

    for (int i = -size; i <= size; i += spacing) {
        // X lines
        vertices.append(QVector3D(i, 0.01f, -size));
        vertices.append(QVector3D(i, 0.01f, size));
        // Z lines
        vertices.append(QVector3D(-size, 0.01f, i));
        vertices.append(QVector3D(size, 0.01f, i));
    }

    QByteArray vertexData(reinterpret_cast<const char*>(vertices.constData()), vertices.size() * sizeof(QVector3D));

    auto *buffer = new Qt3DCore::QBuffer();
    buffer->setUsage(Qt3DCore::QBuffer::StaticDraw); // Correct usage
    buffer->setData(vertexData);

    auto *positionAttr = new Qt3DCore::QAttribute();
    positionAttr->setName(Qt3DCore::QAttribute::defaultPositionAttributeName());
    positionAttr->setVertexBaseType(Qt3DCore::QAttribute::Float);
    positionAttr->setVertexSize(3);
    positionAttr->setAttributeType(Qt3DCore::QAttribute::VertexAttribute);
    positionAttr->setBuffer(buffer);
    positionAttr->setByteStride(3 * sizeof(float));
    positionAttr->setCount(vertices.size());

    auto *geometry = new Qt3DCore::QGeometry();
    geometry->addAttribute(positionAttr);

    auto *renderer = new Qt3DRender::QGeometryRenderer();
    renderer->setGeometry(geometry);
    renderer->setPrimitiveType(Qt3DRender::QGeometryRenderer::Lines);

    // Material (Phong doesn't support lines well)
    auto *material = new Qt3DExtras::QPhongMaterial();
    material->setDiffuse(Qt::lightGray);

    entity->addComponent(renderer);
    entity->addComponent(material);

    return entity;
}

void Scene3DWidget::addEntity(QString ID, Mesh3dEntry entry)
{
    std::string key = ID.toStdString();
    if (entities3D.find(key) != entities3D.end()) {
        qWarning() << "Entity already exists:" << ID;
        return;
    }

    auto *entity = new Qt3DCore::QEntity(rootEntity);

    // Mesh
    //auto *cubeMesh = new Qt3DExtras::QCuboidMesh();
    //entity->addComponent(cubeMesh);
    auto *renderer = new Qt3DRender::QGeometryRenderer();
    renderer->setGeometry(new Qt3DExtras::QCuboidGeometry(entity));
    applyRenderMode(renderer);
    entity->addComponent(renderer);

    // Transform
    auto *transform = new Qt3DCore::QTransform();
    transform->setTranslation(QVector3D(-entry.position->x/1, entry.position->z/1, entry.position->y/1));
    transform->setScale3D(QVector3D(entry.size->x, entry.size->z, entry.size->y));
    transform->setRotationX(entry.rotation->x);
    transform->setRotationY(-entry.rotation->z);
    transform->setRotationZ(entry.rotation->y);
    entity->addComponent(transform);

    // Material
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
        textureCache[texturePath] = texture; // ✅ Save in cache
    }


    auto *mat = new Qt3DExtras::QDiffuseMapMaterial();
    if (texture) {
        mat->setDiffuse(texture);
    } else {
        qWarning() << "❗ Texture is null, skipping diffuse assignment.";
    }

    //mat->setShareable(true);
    mat->setSpecular(QColor(255, 255, 255));          // white shine
    mat->setShininess(0.0f);
    mat->setAmbient(QColor(60, 60, 60));  // soft base glow
        // more = sharper highlight
    entity->addComponent(mat);
    auto *picker = new Qt3DRender::QObjectPicker(entity);
    picker->setHoverEnabled(true);
    entity->addComponent(picker);

    // Connect signal
    connect(picker, &Qt3DRender::QObjectPicker::clicked, this, [=](Qt3DRender::QPickEvent *event){
        // Yaha hum selection handle karenge
        qDebug() << "Clicked Entity ID:" << QString::fromStdString(key);
        emit selectEntityByCursor(ID);
        selectedEntity(QString::fromStdString(key));  // ✅ Tumhara existing selectEntity function
    });



    entry.entity = entity;
    entry.originalMaterial = mat;
    entities3D[key] = entry;


    qDebug() << "Added entity:" << ID;
}

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
    delete entity; // Qt3D deletes components too
    entities3D.erase(key);

    // If the removed entity was selected, clear selection
    if (selectedEntityId == ID) {
        selectedEntityId.clear();
    }
    qDebug() << "Removed entity:" << ID;
}

void Scene3DWidget::selectedEntity(QString ID)
{
    std::string key = ID.toStdString();
    if (entities3D.find(key) == entities3D.end()) return;

    // 🔄 Restore previous entity’s material
    if (!selectedEntityId.isEmpty() && selectedEntityId != ID) {
        std::string prevKey = selectedEntityId.toStdString();
        if (entities3D.find(prevKey) != entities3D.end()){
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

    // 💾 Store original material (once)
    if (!entry.originalMaterial) {
        auto originals = entity->componentsOfType<Qt3DRender::QMaterial>();
        if (!originals.isEmpty()) entry.originalMaterial = originals[0];
    }

    if (entry.originalMaterial)
        entity->removeComponent(entry.originalMaterial);

    // ⚡ Reuse or create the shared highlight
    if (!sharedHighlightMaterial) {
        sharedHighlightMaterial = new Qt3DExtras::QPhongMaterial();
        sharedHighlightMaterial->setAmbient(Qt::yellow);
        sharedHighlightMaterial->setSpecular(Qt::black);
        sharedHighlightMaterial->setShininess(0);
    }

    entity->addComponent(sharedHighlightMaterial);

    qDebug() << "🌟 Highlighted with shared material:" << ID;
}

void Scene3DWidget::clearBuffer(){
    delete sharedHighlightMaterial;
    textureCache.clear();
}

void Scene3DWidget::updateEntities()
{
    for (auto& [key, entry] : entities3D)
    {
        Qt3DCore::QEntity* entity = entry.entity;

        if (!entity) {
            qWarning() << "❗ Null entity for key:" << QString::fromStdString(key);
            continue;
        }

        // 🔁 Update Transform
        auto transforms = entity->componentsOfType<Qt3DCore::QTransform>();
        if (!transforms.isEmpty()) {
            auto *transform = transforms[0];
            if (transform && entry.position && entry.rotation && entry.size) {
                transform->setTranslation(QVector3D(-entry.position->x/1, entry.position->z/1, entry.position->y/1));
                transform->setScale3D(QVector3D(entry.size->x, entry.size->z, entry.size->y));
                transform->setRotationX(entry.rotation->x);
                transform->setRotationY(-entry.rotation->z);
                transform->setRotationZ(entry.rotation->y);
            }
        } else {
            qWarning() << "❗ Transform missing for key:" << QString::fromStdString(key);
        }

        // 🎨 Update Material
        auto materials = entity->componentsOfType<Qt3DExtras::QPhongMaterial>();
        if (!materials.isEmpty()) {
            auto *mat = materials[0];
            if (mat && entry.mesh && entry.mesh->color) {
                //mat->setDiffuse(*entry.mesh->color);
            }
        }
    }

    //qDebug() << "✅ Updated all entities via Mesh3dEntry map.";
}

void Scene3DWidget::MoveEntity(QString ID)
{
    std::string key = ID.toStdString();

    // Check if entity exists
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

    // Find the transform component
    auto transforms = entity->componentsOfType<Qt3DCore::QTransform>();
    if (!transforms.isEmpty()) {
        Qt3DCore::QTransform* transform = transforms[0];

        if (transform && entry.position && entry.rotation && entry.size) {
            transform->setTranslation(QVector3D(-entry.position->x/1, entry.position->z/1, entry.position->y/1));
            transform->setScale3D(QVector3D(entry.size->x, entry.size->z, entry.size->y));
            transform->setRotationX(entry.rotation->x);
            transform->setRotationY(-entry.rotation->z);
            transform->setRotationZ(entry.rotation->y);
            //qDebug() << "✅ Moved entity:" << ID;
        }
    } else {
        qWarning() << "MoveEntity failed. No transform component found for:" << ID;
    }
}

void Scene3DWidget::focusEntity(QString ID) {
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

    // Get position from transform
    auto transforms = entity->componentsOfType<Qt3DCore::QTransform>();
    if (transforms.isEmpty()) {
        qWarning() << "FocusEntity failed. No transform found for:" << ID;
        return;
    }

    QVector3D entityPos = transforms[0]->translation();

    // Move camera behind and above entity (Unity-style focus)
    Qt3DRender::QCamera *camera = view->camera();
    QVector3D offset(0, 3, -5);  // Customize as needed
    camera->setPosition(entityPos + offset);
    camera->setViewCenter(entityPos);

    // 🩷 Also update yaw-pitch to face it directly (optional)
    QVector3D dir = (entityPos - camera->position()).normalized();
    yaw = qRadiansToDegrees(std::atan2(dir.x(), dir.z()));
    pitch = qRadiansToDegrees(std::asin(dir.y()));
}

void Scene3DWidget::updateCamera()
{
    if (!view || !view->camera()) return;

    Qt3DRender::QCamera *camera = view->camera();

    // If an entity is selected, follow it
    if (followSelectedEntity && !selectedEntityId.isEmpty()) {
        std::string key = selectedEntityId.toStdString();
        auto it = entities3D.find(key);
        if (it != entities3D.end() && it->second.entity) {
            auto transforms = it->second.entity->componentsOfType<Qt3DCore::QTransform>();
            if (!transforms.isEmpty()) {
                QVector3D entityPos = transforms[0]->translation();
                // Camera offset: 5 units behind, 3 units above
                QVector3D cameraOffset(5, 3, 0);
                camera->setPosition(entityPos + cameraOffset);
                camera->setViewCenter(entityPos);
                return; // Skip manual camera controls
            }
        }
    }

    //Qt3DRender::QCamera *camera = view->camera();
    QVector3D forward = (camera->viewCenter() - camera->position()).normalized(); // 🔥 Look direction
    QVector3D right = QVector3D::crossProduct(forward, camera->upVector()).normalized();
    QVector3D up = camera->upVector().normalized(); // ☝️ vertical direction

    QVector3D position = camera->position();
    float speed = 0.5f;

    if (pressedKeys.contains(Qt::Key_W)) position += forward * speed;
    if (pressedKeys.contains(Qt::Key_S)) position -= forward * speed;
    if (pressedKeys.contains(Qt::Key_A)) position -= right * speed;
    if (pressedKeys.contains(Qt::Key_D)) position += right * speed;
    if (pressedKeys.contains(Qt::Key_E)) position += up * speed;     // 👆 up
    if (pressedKeys.contains(Qt::Key_Q)) position -= up * speed;     // 👇 down
    if (pressedKeys.contains(Qt::Key_Shift)) speed *= 3.0f;  // Fast mode


    camera->setPosition(position);
    camera->setViewCenter(position + forward); // 🔁 maintain direction
}

void Scene3DWidget::updateCameraRotation()
{
    // Only update rotation if no entity is selected
    if (!selectedEntityId.isEmpty() && followSelectedEntity) return;
    Qt3DRender::QCamera *camera = view->camera();

    QVector3D direction;
    direction.setX(cos(qDegreesToRadians(yaw)) * cos(qDegreesToRadians(-pitch)));
    direction.setY(sin(qDegreesToRadians(pitch)));
    direction.setZ(sin(qDegreesToRadians(yaw)) * cos(qDegreesToRadians(-pitch)));

    QVector3D newCenter = camera->position() + direction.normalized();
    camera->setViewCenter(newCenter);
}

void Scene3DWidget::applyRenderMode(Qt3DRender::QGeometryRenderer* renderer)
{
    switch (currentRenderMode) {
    case RenderMode::Wireframe:
        renderer->setPrimitiveType(Qt3DRender::QGeometryRenderer::Lines); // Edge lines only
        break;
    case RenderMode::WireframeWithDraw:
        renderer->setPrimitiveType(Qt3DRender::QGeometryRenderer::LineStrip); // Continuous wire look
        break;
    case RenderMode::Full:

    default:
        renderer->setPrimitiveType(Qt3DRender::QGeometryRenderer::Triangles); // Normal
        break;
    }
}


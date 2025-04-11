#include "scene.h"
#include "game.h"
#include <QDebug>
#include <QFont>

Scene::Scene(Game *game, QGraphicsScene *scene, QObject *parent)
    : QObject(parent),
      game(game),
      scene(scene)
{
}

Scene::~Scene()
{
}

void Scene::toggleDebugMode()
{
    debugMode = !debugMode;
    qDebug() << "Debug mode" << (debugMode ? "enabled" : "disabled");
    
    if (debugMode) {
        createCoordinateDisplay();
    } else if (coordDisplayItem) {
        scene->removeItem(coordDisplayItem);
        delete coordDisplayItem;
        coordDisplayItem = nullptr;
    }
    
    updateBarrierVisibility();
}

void Scene::createCoordinateDisplay()
{
    if (!coordDisplayItem) {
        coordDisplayItem = scene->addText("Coordinates: (0, 0)");
        coordDisplayItem->setDefaultTextColor(Qt::white);
        coordDisplayItem->setFont(QFont("Arial", 12, QFont::Bold));
        coordDisplayItem->setZValue(100); // Always on top
        
        // Position in top-left corner of view
        coordDisplayItem->setPos(10, 10);
    }
}

void Scene::updateMousePosition(const QPointF &scenePos)
{
    if (debugMode && coordDisplayItem) {
        coordDisplayItem->setPlainText(QString("Coordinates: (%1, %2)").arg(
            static_cast<int>(scenePos.x())).arg(static_cast<int>(scenePos.y())));
    }
}

void Scene::updateBarrierVisibility()
{
    // This is a base implementation
    // Each scene should override this to handle their specific barrier items
    qDebug() << "updateBarrierVisibility called in base Scene class";
}

void Scene::setPlayerPos(QPointF newPos)
{
    // Base implementation (does nothing)
    Q_UNUSED(newPos);
}

void Scene::addBarrier(QGraphicsRectItem* barrier)
{
    // Base implementation (does nothing)
    Q_UNUSED(barrier);
}

void Scene::removeBarrier(QGraphicsRectItem* barrier)
{
    // Base implementation (does nothing)
    Q_UNUSED(barrier);
}

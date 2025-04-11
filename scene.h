#ifndef SCENE_H
#define SCENE_H

#include <QObject>
#include <QGraphicsScene>
#include <QGraphicsItem>
#include <QPointF>
#include <QGraphicsTextItem>

class Game;

class Scene : public QObject
{
    Q_OBJECT

public:
    explicit Scene(Game *game, QGraphicsScene *scene, QObject *parent = nullptr);
    virtual ~Scene();

    virtual void initialize() = 0;
    virtual void handleKeyPress(int key) = 0;
    virtual void cleanup() = 0;
    
    // Debug helpers
    void toggleDebugMode(); // Turn debug mode on/off
    bool isDebugModeEnabled() const { return debugMode; }
    void updateMousePosition(const QPointF &scenePos); // Update the debug coordinate display

    virtual void setPlayerPos(QPointF newPos);
    virtual void addBarrier(QGraphicsRectItem* barrier);
    virtual void removeBarrier(QGraphicsRectItem* barrier);
    virtual void updateBarrierVisibility(); // Show/hide barrier outlines

protected:
    Game *game;
    QGraphicsScene *scene;
    
    // Debug related elements
    bool debugMode = false;
    QGraphicsTextItem *coordDisplayItem = nullptr;
    void createCoordinateDisplay();
};

#endif // SCENE_H

#ifndef SCENE_H
#define SCENE_H

#include <QObject>
#include <QGraphicsScene>

class Game;

class Scene : public QObject
{
    Q_OBJECT
public:
    explicit Scene(Game *game, QGraphicsScene *scene, QObject *parent = nullptr);
    virtual ~Scene();

    virtual void initialize() = 0;
    virtual void cleanup() = 0;
    virtual void handleKeyPress(int key) = 0;

protected:
    Game *game;
    QGraphicsScene *scene;
};

#endif // SCENE_H

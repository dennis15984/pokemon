#include "scene.h"
#include "game.h"

Scene::Scene(Game *game, QGraphicsScene *scene, QObject *parent)
    : QObject(parent),
      game(game),
      scene(scene)
{
}

Scene::~Scene()
{
}

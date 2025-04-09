#ifndef LABORATORYSCENE_H
#define LABORATORYSCENE_H

#include "scene.h"
#include <QGraphicsPixmapItem>
#include <QGraphicsRectItem>
#include <QGraphicsTextItem>
#include <QList>
#include <QTimer>
#include <QDirIterator>

class LaboratoryScene : public Scene
{
    Q_OBJECT

public:
    explicit LaboratoryScene(Game *game, QGraphicsScene *scene, QObject *parent = nullptr);
    ~LaboratoryScene() override;

    void initialize() override;
    void cleanup() override;
    void handleKeyPress(int key) override;

private slots:
    void updateScene();

private:
    QGraphicsPixmapItem *backgroundItem;
    QGraphicsPixmapItem *playerItem;
    QGraphicsPixmapItem *npcItem;
    QGraphicsPixmapItem *labTableItem;
    QList<QGraphicsPixmapItem*> pokeBallItems;
    QList<QGraphicsRectItem*> barrierItems;
    QGraphicsPixmapItem *transitionPoint;
    QGraphicsTextItem *transitionNumberText;
    QGraphicsRectItem *transitionNumberBg;

    QTimer *updateTimer;

    // Player position and camera
    QPointF playerPos;
    QPointF cameraPos;

    // Lab dimensions
    const int LAB_WIDTH = 438;
    const int LAB_HEIGHT = 455;

    // View dimensions
    const int VIEW_WIDTH = 525;
    const int VIEW_HEIGHT = 450;

    void createBackground();
    void createNPC();
    void createPlayer();
    void createLabTable();
    void createBarriers();
    void createTransitionPoint();
    void updateCamera();
    void showDialogue(const QString &text);
    void printAvailableResources();
};

#endif // LABORATORYSCENE_H

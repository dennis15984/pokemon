#ifndef GRASSLANDSCENE_H
#define GRASSLANDSCENE_H

#include "scene.h"
#include <QGraphicsScene>
#include <QGraphicsPixmapItem>
#include <QGraphicsRectItem>
#include <QTimer>
#include <QSet>
#include <QRandomGenerator>

class Game;

class GrasslandScene : public Scene
{
    Q_OBJECT

public:
    explicit GrasslandScene(Game *game, QGraphicsScene *scene, QObject *parent = nullptr);
    ~GrasslandScene() override;

    void initialize() override;
    void handleKeyPress(int key) override;
    void handleKeyRelease(int key);
    void cleanup() override;

protected:
    void updateBarrierVisibility() override; // Override for debug visualization

private slots:
    void updateScene();
    void processMovement();

private:
    // Constants for grassland dimensions
    const int GRASSLAND_WIDTH = 1000;
    const int GRASSLAND_HEIGHT = 1667;

    // Timers
    QTimer *updateTimer{nullptr};
    QTimer *movementTimer{nullptr};

    // Graphics items
    QGraphicsPixmapItem *backgroundItem{nullptr};
    QGraphicsPixmapItem *playerItem{nullptr};
    QVector<QGraphicsRectItem*> barrierItems;
    QGraphicsRectItem *townPortalItem{nullptr};  // Portal to return to town
    QGraphicsRectItem *bulletinBoardItem{nullptr};  // Bulletin board for conversation
    
    // Dialogue items
    QGraphicsItem* dialogBoxItem{nullptr};
    QGraphicsTextItem* dialogTextItem{nullptr};
    bool isDialogueActive{false};
    int currentDialogueState{0};

    // Bag items
    QGraphicsPixmapItem* bagBackgroundItem{nullptr};
    QVector<QGraphicsPixmapItem*> bagPokemonSprites;
    QVector<QGraphicsTextItem*> bagPokemonNames;
    QVector<QGraphicsRectItem*> bagSlotRects;
    bool isBagOpen{false};

    // Player state
    QPointF playerPos{500, 500}; // Default starting position (center of grassland)
    QPointF cameraPos{0, 0}; // Camera position for viewing
    QString playerDirection{"F"}; // F=front, B=back, L=left, R=right
    int walkFrame{0};
    
    // Input handling
    QSet<int> pressedKeys;
    int currentPressedKey{0};

    // Methods
    void createBackground();
    void createPlayer();
    void createBarriers();
    void updatePlayerSprite();
    void updatePlayerPosition();
    void updateCamera();
    bool checkCollision();
    void toggleBag();
    void updateBagDisplay();
    void clearBagDisplayItems();
    void showDialogueBox(const QString &text);
    void showDialogue(const QString &text);
    void closeDialogue();
    void handleDialogue();
    bool isPlayerNearTownPortal() const;
    bool isPlayerNearBulletinBoard() const;
};

#endif // GRASSLANDSCENE_H 
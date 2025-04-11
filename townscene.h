#ifndef TOWNSCENE_H
#define TOWNSCENE_H

#include "scene.h"
#include <QGraphicsScene>
#include <QGraphicsPixmapItem>
#include <QGraphicsRectItem>
#include <QTimer>
#include <QSet>
#include <QVector>
#include <QMap>

class Game;

class TownScene : public Scene
{
    Q_OBJECT

public:
    explicit TownScene(Game *game, QGraphicsScene *scene, QObject *parent = nullptr);
    ~TownScene() override;

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
    // Constants for town dimensions
    const int TOWN_WIDTH = 1000;
    const int TOWN_HEIGHT = 1000;

    // Timers
    QTimer *updateTimer{nullptr};
    QTimer *movementTimer{nullptr};

    // Graphics items
    QGraphicsPixmapItem *backgroundItem{nullptr};
    QGraphicsPixmapItem *playerItem{nullptr};
    QVector<QGraphicsRectItem*> barrierItems;
    QVector<QGraphicsRectItem*> bulletinBoardItems;
    QGraphicsRectItem *labPortalItem{nullptr};  // Portal to return to lab
    QGraphicsRectItem *grasslandPortalItem{nullptr};  // Portal to grassland
    
    // Box items - new
    QVector<QGraphicsPixmapItem*> boxItems;
    QVector<QGraphicsRectItem*> boxHitboxes;
    QMap<int, bool> boxOpened;  // Track which boxes have been opened
    
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
    QPointF playerPos{500, 500}; // Default starting position (center of 1000x1000 town)
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
    void createBoxes();  // New method to create boxes
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
    bool isPlayerNearBulletinBoard(int &boardIndex) const;
    bool isPlayerNearLabPortal() const;
    bool isPlayerNearGrasslandPortal() const;
    bool isPlayerNearBox(int &boxIndex) const;  // New method to check proximity to boxes
};

#endif // TOWNSCENE_H 
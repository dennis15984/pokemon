#ifndef LABORATORYSCENE_H
#define LABORATORYSCENE_H

#include "scene.h"
#include <QGraphicsPixmapItem>
#include <QGraphicsRectItem>
#include <QGraphicsTextItem>
#include <QVector>
#include <QTimer>
#include <QDirIterator>
#include <QSet>

class LaboratoryScene : public Scene
{
    Q_OBJECT

public:
    explicit LaboratoryScene(Game *game, QGraphicsScene *scene, QObject *parent = nullptr);
    ~LaboratoryScene();

    void initialize() override;
    void cleanup() override;
    void handleKeyPress(int key) override;
    void handleKeyRelease(int key) override;
    void update() override;

protected:
    void updatePlayerSprite();
    bool checkCollision();
    void updatePlayerPosition();

private:
    static const int SCENE_WIDTH = 750;  // Total scene width (black background)
    static const int SCENE_HEIGHT = 750; // Total scene height (black background)
    static const int LAB_WIDTH = 438;    // Lab width - updated to match actual image size
    static const int LAB_HEIGHT = 550;   // Lab height - increased to ensure proper scrolling
    static const int VIEW_WIDTH = 525;   // Window width from requirements
    static const int VIEW_HEIGHT = 450;  // Window height from requirements

    QGraphicsPixmapItem* backgroundItem{nullptr};
    QGraphicsPixmapItem* playerItem{nullptr};
    QGraphicsPixmapItem* npcItem{nullptr};
    QGraphicsPixmapItem* labTableItem{nullptr};
    QVector<QGraphicsPixmapItem*> pokeBallItems;
    QVector<QGraphicsRectItem*> barrierItems;
    QGraphicsRectItem* transitionBoxItem{nullptr}; // Area that transitions to Town scene
    
    // Bag related items
    QGraphicsPixmapItem* bagBackgroundItem{nullptr}; // Store bag background separately
    QVector<QGraphicsPixmapItem*> bagPokemonSprites;
    QVector<QGraphicsTextItem*> bagPokemonNames;
    QVector<QGraphicsItem*> bagSlotRects; // Change to store generic QGraphicsItem*
    bool isBagOpen{false};

    // Player animation and movement
    int walkFrame{0};
    QString playerDirection{"F"};
    QTimer* updateTimer{nullptr};
    QTimer* movementTimer{nullptr};
    int currentPressedKey{0};
    QSet<int> pressedKeys;  // Set to track currently pressed keys

    // Player position and camera
    QPointF playerPos{220, 350};
    QPointF cameraPos{0, 0};

    QGraphicsItem* dialogBoxItem{nullptr};
    QGraphicsTextItem* dialogTextItem{nullptr};
    int currentDialogueState{0};
    bool isDialogueActive{false};

    // Pokémon selection
    bool pokemonSelectionActive{false};
    int selectedPokemonType{-1}; // -1: None, 0: Squirtle, 1: Charmander, 2: Bulbasaur
    bool hasChosenPokemon{false}; // Flag to track if player has already chosen a Pokémon

    void createBackground();
    void createNPC();
    void createPlayer();
    void createLabTable();
    void createBarriers();
    void createTransitionPoint();
    void updateCamera();
    void showDialogue(const QString &text);
    void printAvailableResources();

    // Bag functions
    void toggleBag();
    void updateBagDisplay();
    void clearBagDisplayItems();

    void showDialogueBox(const QString &text);
    void handleDialogue();
    bool isPlayerNearNPC() const;
    bool isPlayerNearDoor() const;
    bool isPlayerNearPokeball(int &ballIndex) const;
    bool isPlayerOnTransitionArea() const; // Check if player is on the transition area
    void closeDialogue();
    void updateScene();
    void processMovement();
    void centerLabInitially();
    void handlePokemonSelection(int key);
    void choosePokemon(int pokemonIndex);
    void startPokemonSelection();
};

#endif // LABORATORYSCENE_H

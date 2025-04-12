#ifndef GRASSLANDSCENE_H
#define GRASSLANDSCENE_H

#include "scene.h"
#include "pokemon.h"
#include <QGraphicsScene>
#include <QGraphicsPixmapItem>
#include <QGraphicsRectItem>
#include <QTimer>
#include <QSet>
#include <QRandomGenerator>
#include <QMap>

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
    void update();

protected:
    void updateBarrierVisibility() override; // Override for debug visualization

private slots:
    void updateScene();
    void processMovement();

private:
    // Constants for grassland dimensions
    const int GRASSLAND_WIDTH = 1000;
    const int GRASSLAND_HEIGHT = 1667;

    // Battle menu options
    enum BattleOption {
        FIGHT = 0,
        BAG = 1,
        POKEMON = 2,
        RUN = 3
    };
    BattleOption selectedBattleOption = FIGHT;  // Changed from int to BattleOption

    // Battle menu items
    QVector<QGraphicsItem*> battleMenuRects;
    QVector<QGraphicsTextItem*> battleMenuTexts;
    bool isMoveSelectionActive{false};  // New flag for move selection

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
    bool isPokemonSelectionDialogue{false};
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

    // Ledge items for one-way barriers (can jump down, can't climb up)
    QVector<QGraphicsRectItem*> ledgeItems;
    
    // Wild Pokémon encounter struct
    struct WildPokemon {
        QString type;                     // Pokémon type (Bulbasaur, Charmander, Squirtle)
        QPointF position;                 // Position in the scene
        QGraphicsPixmapItem* spriteItem;  // Sprite item in the scene
        bool encountered;                 // Whether this Pokémon has been encountered
    };

    // Tall grass areas for wild Pokémon encounters
    QVector<QGraphicsRectItem*> tallGrassItems;
    
    // Wild Pokémon data
    QVector<WildPokemon> wildPokemons;
    
    // Grass area tracking
    QMap<int, bool> grassAreaVisited;   // Maps grass area index to visited status
    int currentGrassArea;               // Index of current grass area (-1 if not in grass)
    
    // Battle scene elements
    bool inBattleScene{false};
    bool isBattleBagOpen{false};
    QGraphicsPixmapItem* battleSceneItem{nullptr};
    QString currentBattlePokemonType;
    
    // Battle mechanics
    Pokemon* wildPokemon{nullptr};  // Store the current wild Pokemon
    int wildPokemonHp{30};         // Wild Pokemon's current HP
    bool isPlayerTurn{true};       // Track whose turn it is
    QTimer* battleTimer{nullptr};  // Timer for battle animations and delays
    
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
    bool isPlayerJumpingDownLedge(const QPointF& newPos) const;
    void createTallGrassAreas();
    void spawnWildPokemon(int grassAreaIndex);
    bool isPlayerInGrassArea(int* areaIndex = nullptr);
    void checkWildPokemonCollision();
    void startBattle(const QString& pokemonType);
    void showBattleScene();
    void showBattleBag();
    void showMoveSelection();
    void exitBattleScene();
    void showPokemonSelectionDialogue(const QString& text);
    void handleBagSelection(int itemIndex);
    
    // Battle mechanics methods
    void handleMoveSelection(int moveIndex);  // Handle when player selects a move
    void calculateDamage(Pokemon* attacker, const Pokemon::Move& move, Pokemon* defender);  // Calculate damage
    void applyDamage(int damage, bool toWildPokemon);  // Apply damage to either Pokemon
    void wildPokemonTurn();  // Handle wild Pokemon's turn
    void checkBattleEnd();   // Check if battle should end
    void showBattleMessage(const QString& message);  // Show battle messages
};

#endif // GRASSLANDSCENE_H 
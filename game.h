#ifndef GAME_H
#define GAME_H

#include <QObject>
#include <QGraphicsScene>
#include <QKeyEvent>
#include <QMap>
#include <QString>
#include <memory>

// Forward declarations
class Scene;
class TitleScene;
class LaboratoryScene;
class TownScene;
class GrasslandScene;
class BattleScene;
class Player;
class Pokemon;
class Item;

// Game states
enum class GameState {
    TITLE,
    LABORATORY,
    TOWN,
    GRASSLAND,
    BATTLE
};

class Game : public QObject
{
    Q_OBJECT

public:
    explicit Game(QGraphicsScene *scene, QObject *parent = nullptr);
    ~Game();

    // Game lifecycle methods
    void start();
    void pause();
    void resume();
    void exit();

    // Scene management
    void changeScene(GameState newState);
    Scene* getCurrentScene() const;

    // Event handling
    void handleKeyPress(QKeyEvent *event);
    void handleKeyRelease(QKeyEvent *event);

    // Player and Pokémon management
    Player* getPlayer() const;
    void addPokemon(Pokemon* pokemon);
    void addItem(const QString& itemName, int quantity);
    QList<Pokemon*> getPokemons() const;
    QMap<QString, int> getItems() const;

    // Battle management
    void startBattle(Pokemon* wildPokemon);
    void endBattle(bool playerWon);

    // Utility functions
    bool hasCompletedLaboratory() const;
    void setLaboratoryCompleted(bool completed);

private:
    // Core components
    QGraphicsScene *gameScene;
    Scene *currentScene;
    GameState currentState;

    // Game scenes
    TitleScene *titleScene;
    LaboratoryScene *laboratoryScene;
    TownScene *townScene;
    GrasslandScene *grasslandScene;
    BattleScene *battleScene;

    // Game data
    Player *player;
    QList<Pokemon*> playerPokemons;
    QMap<QString, int> inventory;

    // Game state flags
    bool laboratoryCompleted;

    // Initialize different game components
    void initScenes();
    void cleanup();
};

#endif // GAME_H

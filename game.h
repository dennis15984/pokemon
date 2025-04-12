#ifndef GAME_H
#define GAME_H

#include <QObject>
#include <QGraphicsView>
#include <QGraphicsScene>
#include <QKeyEvent>
#include <QMap>
#include <QString>
#include <memory>
#include "pokemon.h"
#include <QVector>
#include <QDebug>
#include <QPointF>

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
    explicit Game(QGraphicsScene* scene, QObject *parent = nullptr);
    ~Game();

    // Game lifecycle methods
    void start();
    void pause();
    void resume();
    void exit();

    // Scene management
    void changeScene(GameState state);
    Scene* getCurrentScene() const;

    // Event handling
    void handleKeyPress(QKeyEvent *event);
    void handleKeyRelease(QKeyEvent *event);

    // Player and Pokémon management
    Player* getPlayer() const;
    void addPokemon(Pokemon* pokemon);
    void addItem(const QString& itemName, int quantity);
    QVector<Pokemon*> getPokemons() const { return playerPokemon; }
    QMap<QString, int> getItems() const;
    void setItems(const QMap<QString, int>& items);
    const QVector<Pokemon*>& getPokemon() const { 
        qDebug() << "Player has" << playerPokemon.size() << "Pokémon";
        return playerPokemon; 
    }
    void generateRandomPokeballs();
    Pokemon* getPokemonAtBall(int ballIndex) const;

    // Battle management
    void startBattle(Pokemon* wildPokemon);
    void endBattle(bool playerWon);

    // Utility functions
    bool hasCompletedLaboratory() const;
    void setLaboratoryCompleted(bool completed);

    // New methods to handle town boxes
    const QVector<QPointF>& getTownBoxPositions() const;
    const QMap<int, bool>& getTownBoxOpenedStates() const;
    const QMap<int, QString>& getTownBoxContents() const;
    void setTownBoxOpenedState(int boxIndex, bool isOpened);
    bool areTownBoxesInitialized() const;
    void generateTownBoxes();

private:
    // Core components
    QGraphicsScene* scene;
    Scene* currentScene;
    GameState currentState;

    // Game scenes
    TitleScene* titleScene;
    LaboratoryScene* laboratoryScene;
    TownScene* townScene;
    GrasslandScene* grasslandScene;
    BattleScene* battleScene;

    // Game data
    Player* player;
    QMap<QString, int> inventory;

    // Game state flags
    bool laboratoryCompleted;

    // Store player's pokemon
    QVector<Pokemon*> playerPokemon;
    // Store the pokemon assigned to each pokeball
    QVector<Pokemon*> pokeballPokemon;

    // Town boxes data - new
    QVector<QPointF> townBoxPositions;
    QMap<int, bool> townBoxOpenedStates;
    QMap<int, QString> townBoxContents;
    bool townBoxesInitialized = false;

    // Initialize different game components
    void initScenes();
    void cleanup();
};

#endif // GAME_H

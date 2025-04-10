#include "game.h"
#include "scene.h"
#include "titlescene.h"
#include "laboratoryscene.h"
#include "townscene.h"
#include "grasslandscene.h"
#include <QDebug>
#include <QRandomGenerator>

Game::Game(QGraphicsScene* scene, QObject *parent)
    : QObject(parent),
      scene(scene),
      currentScene(nullptr),
      currentState(GameState::TITLE),
      titleScene(nullptr),
      laboratoryScene(nullptr),
      townScene(nullptr),
      grasslandScene(nullptr),
      battleScene(nullptr),
      player(nullptr),
      laboratoryCompleted(false)
{
    // No need to create view or scene, they are passed in from MainWindow
    
    qDebug() << "Game initialized";
}

Game::~Game()
{
    // Don't delete scene or view, they are owned by MainWindow
    cleanup();
}

void Game::start()
{
    // Start with title scene
    changeScene(GameState::TITLE);
    
    qDebug() << "Game started";
}

void Game::pause()
{
    qDebug() << "Game paused";
}

void Game::resume()
{
    qDebug() << "Game resumed";
}

void Game::exit()
{
    qDebug() << "Game exited";
}

void Game::changeScene(GameState state)
{
    // Clean up old scene if it exists
    if (currentScene) {
        currentScene->cleanup();
        // Don't delete the scene object itself, just clean up its contents
    }

    currentState = state;
    qDebug() << "Changed to scene:" << static_cast<int>(state);

    // Create or reuse existing scene
    switch (state) {
        case GameState::TITLE:
            qDebug() << "Setting current scene to Title scene";
            if (!titleScene) {
                titleScene = new TitleScene(this, scene);
                // Simple one-time connection since we won't return to title scene
                connect(titleScene, &TitleScene::startGame, this, [this]() {
                    changeScene(GameState::LABORATORY);
                });
            }
            currentScene = titleScene;
            break;
        case GameState::LABORATORY:
            qDebug() << "Setting current scene to Laboratory scene";
            if (!laboratoryScene) {
                laboratoryScene = new LaboratoryScene(this, scene);
            }
            currentScene = laboratoryScene;
            generateRandomPokeballs(); // Generate random pokemon for pokeballs
            break;
        case GameState::TOWN:
            qDebug() << "Setting current scene to Town scene";
            if (!townScene) {
                townScene = new TownScene(this, scene);
            }
            currentScene = townScene;
            break;
        case GameState::GRASSLAND:
            qDebug() << "Setting current scene to Grassland scene";
            if (!grasslandScene) {
                grasslandScene = new GrasslandScene(this, scene);
            }
            currentScene = grasslandScene;
            break;
        case GameState::BATTLE:
            // Will be implemented later
            qDebug() << "Battle scene not yet implemented";
            break;
        default:
            qDebug() << "Unknown scene state";
            return;
    }

    // Initialize the new current scene
    if (currentScene) {
        qDebug() << "Initializing new scene";
        currentScene->initialize();
        qDebug() << "Scene initialization complete";
    }
}

Scene* Game::getCurrentScene() const
{
    return currentScene;
}

void Game::handleKeyPress(QKeyEvent *event)
{
    qDebug() << "Game received key press event - key:" << event->key() << "text:" << event->text();
    
    // Pass key events to the current scene
    if (currentScene) {
        currentScene->handleKeyPress(event->key());
    }
}

void Game::handleKeyRelease(QKeyEvent *event)
{
    if (currentScene) {
        // Handle key releases for laboratory and town scenes
        if (LaboratoryScene* labScene = dynamic_cast<LaboratoryScene*>(currentScene)) {
            labScene->handleKeyRelease(event->key());
        }
        else if (TownScene* tScene = dynamic_cast<TownScene*>(currentScene)) {
            tScene->handleKeyRelease(event->key());
        }
        else if (GrasslandScene* gScene = dynamic_cast<GrasslandScene*>(currentScene)) {
            gScene->handleKeyRelease(event->key());
        }
    }
}

Player* Game::getPlayer() const
{
    return player;
}

void Game::addPokemon(Pokemon* pokemon)
{
    if (pokemon) {
        playerPokemon.append(pokemon);
        qDebug() << "Added" << pokemon->getName() << "to player's collection";
    }
}

void Game::addItem(const QString& itemName, int quantity)
{
    inventory[itemName] += quantity;
    qDebug() << "Added" << quantity << "of" << itemName;
}

QMap<QString, int> Game::getItems() const
{
    return inventory;
}

void Game::startBattle(Pokemon* wildPokemon)
{
    // This will be expanded once we implement the Battle scene
    qDebug() << "Battle started";
}

void Game::endBattle(bool playerWon)
{
    qDebug() << "Battle ended, player" << (playerWon ? "won" : "lost");
}

bool Game::hasCompletedLaboratory() const
{
    return laboratoryCompleted;
}

void Game::setLaboratoryCompleted(bool completed)
{
    laboratoryCompleted = completed;
}

void Game::initScenes()
{
    // Initialize scenes that weren't created in the constructor
    // We've already created the initial scenes in the constructor
    qDebug() << "Additional scenes initialized";
}

void Game::cleanup()
{
    // Clean up all scenes
    if (titleScene) {
        delete titleScene;
        titleScene = nullptr;
    }

    if (laboratoryScene) {
        delete laboratoryScene;
        laboratoryScene = nullptr;
    }

    if (townScene) {
        delete townScene;
        townScene = nullptr;
    }

    if (grasslandScene) {
        delete grasslandScene;
        grasslandScene = nullptr;
    }

    if (battleScene) {
        delete battleScene;
        battleScene = nullptr;
    }

    // Clean up pokemon
    qDeleteAll(playerPokemon);
    playerPokemon.clear();
    qDeleteAll(pokeballPokemon);
    pokeballPokemon.clear();

    // Other cleanups will be added as we implement more classes
    qDebug() << "Game resources cleaned up";
}

void Game::generateRandomPokeballs() {
    qDebug() << "Starting to generate random pokemon for pokeballs...";
    
    // Clear existing pokeball assignments
    qDeleteAll(pokeballPokemon);
    pokeballPokemon.clear();
    
    // Create list of available pokemon types
    QVector<Pokemon::Type> types = {
        Pokemon::CHARMANDER,
        Pokemon::SQUIRTLE,
        Pokemon::BULBASAUR
    };
    
    qDebug() << "Available pokemon types:" << types.size();
    
    // Randomly assign pokemon to each pokeball
    while (!types.isEmpty() && pokeballPokemon.size() < 3) {
        int index = QRandomGenerator::global()->bounded(types.size());
        qDebug() << "Creating pokemon of type index:" << index;
        Pokemon* pokemon = new Pokemon(types[index]);
        pokeballPokemon.append(pokemon);
        types.removeAt(index);
    }
    
    qDebug() << "Generated random pokemon for pokeballs:";
    for (int i = 0; i < pokeballPokemon.size(); ++i) {
        qDebug() << "Ball" << i << "contains" << pokeballPokemon[i]->getName();
    }
}

Pokemon* Game::getPokemonAtBall(int ballIndex) const {
    qDebug() << "Getting pokemon at ball index:" << ballIndex;
    qDebug() << "Number of pokemon in pokeballPokemon:" << pokeballPokemon.size();
    
    if (ballIndex >= 0 && ballIndex < pokeballPokemon.size()) {
        Pokemon* pokemon = pokeballPokemon[ballIndex];
        if (pokemon) {
            qDebug() << "Found pokemon:" << pokemon->getName();
            return pokemon;
        } else {
            qDebug() << "Pokemon at index" << ballIndex << "is null";
        }
    } else {
        qDebug() << "Ball index" << ballIndex << "is out of range";
    }
    return nullptr;
}

#include "game.h"
#include "titlescene.h"
#include <QDebug>

Game::Game(QGraphicsScene *scene, QObject *parent)
    : QObject(parent),
      gameScene(scene),
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
    // Initialize title scene
    titleScene = new TitleScene(this, gameScene);
    qDebug() << "Game initialized";
}

Game::~Game()
{
    cleanup();
}

void Game::start()
{
    qDebug() << "Game started";
    // Show the title scene when game starts
    changeScene(GameState::TITLE);
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

void Game::changeScene(GameState newState)
{
    // Clean up current scene if there is one
    if (currentScene) {
        // Will be implemented later when we have a Scene base class
    }

    currentState = newState;
    qDebug() << "Changed to scene:" << static_cast<int>(newState);

    // Initialize the appropriate scene
    switch (currentState) {
        case GameState::TITLE:
            if (titleScene) {
                titleScene->initialize();
            }
            break;
        // Other scenes will be handled later
        default:
            break;
    }
}

Scene* Game::getCurrentScene() const
{
    return currentScene;
}

void Game::handleKeyPress(QKeyEvent *event)
{
    // Pass key events to the current scene
    if (currentState == GameState::TITLE && titleScene) {
        titleScene->handleKeyPress(event->key());
    }
    qDebug() << "Key pressed:" << event->key();
}

void Game::handleKeyRelease(QKeyEvent *event)
{
    qDebug() << "Key released:" << event->key();
}

Player* Game::getPlayer() const
{
    return player;
}

void Game::addPokemon(Pokemon* pokemon)
{
    // This will be implemented once we have the Pokemon class
    qDebug() << "Pokemon added";
}

void Game::addItem(const QString& itemName, int quantity)
{
    inventory[itemName] += quantity;
    qDebug() << "Added" << quantity << "of" << itemName;
}

QList<Pokemon*> Game::getPokemons() const
{
    return playerPokemons;
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
    // This will be implemented once we have the Scene classes
    qDebug() << "Scenes initialized";
}

void Game::cleanup()
{
    // Clean up all scenes
    if (titleScene) {
        delete titleScene;
        titleScene = nullptr;
    }

    // Other cleanups will be added as we implement more classes
    qDebug() << "Game resources cleaned up";
}

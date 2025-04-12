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
      laboratoryCompleted(false),
      townBoxesInitialized(false)
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
    qDebug() << "Changing scene from" << static_cast<int>(currentState) << "to" << static_cast<int>(state);
    
    // Clean up old scene if it exists
    if (currentScene) {
        qDebug() << "Cleaning up old scene before changing to new scene";
        currentScene->cleanup();
        // Store the previous scene for debugging
        Scene* prevScene = currentScene;
        currentScene = nullptr; // Set to null first to avoid double pointer issues
        
        // Clear the graphics scene to remove all items
        qDebug() << "Clearing graphics scene";
        try {
            scene->clear();
            qDebug() << "Graphics scene cleared successfully";
        } catch (const std::exception& e) {
            qDebug() << "Error clearing graphics scene:" << e.what();
        } catch (...) {
            qDebug() << "Unknown error clearing graphics scene";
        }
    } else {
        // If there's no current scene, just clear the graphics scene
        qDebug() << "No current scene, just clearing graphics scene";
        scene->clear();
    }

    currentState = state;
    qDebug() << "Scene state changed to:" << static_cast<int>(state);

    // If this is the first time entering town, generate the boxes
    if (state == GameState::TOWN && !townBoxesInitialized) {
        generateTownBoxes();
    }

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
        try {
            currentScene->initialize();
            qDebug() << "Scene initialization complete";
        } catch (const std::exception& e) {
            qDebug() << "Error initializing scene:" << e.what();
        } catch (...) {
            qDebug() << "Unknown error initializing scene";
        }
    } else {
        qDebug() << "Failed to set current scene!";
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

const QVector<QPointF>& Game::getTownBoxPositions() const {
    return townBoxPositions;
}

const QMap<int, bool>& Game::getTownBoxOpenedStates() const {
    return townBoxOpenedStates;
}

const QMap<int, QString>& Game::getTownBoxContents() const {
    return townBoxContents;
}

void Game::setTownBoxOpenedState(int boxIndex, bool isOpened) {
    if (boxIndex >= 0 && boxIndex < townBoxPositions.size()) {
        townBoxOpenedStates[boxIndex] = isOpened;
        qDebug() << "Box" << boxIndex << "set to" << (isOpened ? "opened" : "unopened");
    }
}

bool Game::areTownBoxesInitialized() const {
    return townBoxesInitialized;
}

void Game::generateTownBoxes() {
    qDebug() << "Generating town boxes positions";
    
    // Clear existing box data
    townBoxPositions.clear();
    townBoxOpenedStates.clear();
    townBoxContents.clear();
    
    // Define the town dimensions - must match TownScene
    const int TOWN_WIDTH = 1000;
    const int TOWN_HEIGHT = 1000;
    
    // Get list of barrier positions (approximate from TownScene's createBarriers method)
    QVector<QRect> barrierRects = {
        // Tree barriers around the perimeter
        QRect(0, 0, 492,100),  // Top left trees
        QRect(585, 0, 470,100),  // Top right trees
        QRect(0, 0, 80, TOWN_HEIGHT),  // Left trees
        QRect(TOWN_WIDTH - 87, 0, 100, TOWN_HEIGHT),  // Right trees
       
        // Upper buildings (houses)
        QRect(205, 175, 210, 219),  // Left house
        QRect(586, 175, 210, 219),  // Right house
        QRect(173, 326, 31, 68),  // Mailbox beside Left house
        QRect(550, 326, 31, 68),  // Mailbox beside Right house

        // Center-left fence 
        QRect(208, 549, 214, 46),  
        // Bottom fence
        QRect(546, 801, 249, 41),
        
        QRect(550, 470, 281, 225),  // Center main building
        
        QRect(297, 849, 152, 145),  // Lake at bottom
    };
    
    // Define bulletin board positions separately for special handling
    QVector<QRect> bulletinBoardRects = {
        QRect(209, 698, 42, 46),  // Bottom-left bulletin board
        QRect(377, 548, 45, 45),  // Center-left fence bulletin board
        QRect(669, 801, 45, 45),  // Bottom fence bulletin board
    };
    
    // Also add portals to avoid
    QVector<QRect> portalRects = {
        QRect(669, 700, 45, 45),    // Lab portal
        QRect(490, 0, 90, 90),      // Grassland portal
    };
    
    // Create 15 box positions using similar logic to TownScene::createBoxes
    const int BOX_SIZE = 30;
    
    // Helper to check if a position overlaps with barriers
    auto overlapsBarrier = [&barrierRects](const QRectF &rect) -> bool {
        for (const QRect &barrier : barrierRects) {
            if (rect.intersects(barrier)) {
                return true;
            }
        }
        return false;
    };
    
    // Helper to check if a position is too close to a bulletin board or portal
    auto isTooCloseToInteractive = [&bulletinBoardRects, &portalRects](const QRectF &rect) -> bool {
        // Check if too close to bulletin boards (use expanded area to ensure distance)
        for (const QRect &board : bulletinBoardRects) {
            QRectF expandedBoard = board.adjusted(-60, -60, 60, 60); // 60 pixel safety margin
            if (rect.intersects(expandedBoard)) {
                return true;
            }
        }
        
        // Check if too close to portals
        for (const QRect &portal : portalRects) {
            QRectF expandedPortal = portal.adjusted(-40, -40, 40, 40); // 40 pixel safety margin
            if (rect.intersects(expandedPortal)) {
                return true;
            }
        }
        
        return false;
    };
    
    // Helper to check if a position overlaps with other boxes
    auto overlapsExistingBox = [&](const QPointF &pos) -> bool {
        QRectF newRect(pos.x(), pos.y(), BOX_SIZE, BOX_SIZE);
        for (const QPointF &existingPos : townBoxPositions) {
            QRectF existingRect(existingPos.x(), existingPos.y(), BOX_SIZE, BOX_SIZE);
            if (newRect.intersects(existingRect.adjusted(-40, -40, 40, 40))) { // Add some spacing
                return true;
            }
        }
        return false;
    };
    
    // Try to place boxes in valid positions
    int attempts = 0;
    
    // Use the global QRandomGenerator which is seeded randomly at application start
    // This ensures different box positions each time the game is run
    QRandomGenerator *rng = QRandomGenerator::global();
    
    while (townBoxPositions.size() < 15 && attempts < 2000) { // Increased max attempts
        // Generate position with random generator
        int x = rng->bounded(100, TOWN_WIDTH - 100);
        int y = rng->bounded(100, TOWN_HEIGHT - 100);
        QPointF pos(x, y);
        
        // Check if position is valid (not overlapping barriers, bulletin boards, or existing boxes)
        QRectF boxRect(x, y, BOX_SIZE, BOX_SIZE);
        if (!overlapsBarrier(boxRect) && 
            !isTooCloseToInteractive(boxRect) && 
            !overlapsExistingBox(pos)) {
            townBoxPositions.append(pos);
            townBoxOpenedStates[townBoxPositions.size() - 1] = false; // All boxes start unopened
        }
        
        attempts++;
    }
    
    qDebug() << "Generated" << townBoxPositions.size() << "town box positions after" << attempts << "attempts";
    
    // Now, assign items to boxes - only 3 boxes should contain Poké Balls
    // The remaining 12 boxes should only contain Ether or Potion randomly
    QVector<QString> possibleItems = {
        "Ether", "Potion"
    };
    
    // Choose 3 random boxes to contain Poké Balls
    QSet<int> pokeballBoxes;
    while (pokeballBoxes.size() < 3) {
        int boxIndex = rng->bounded(townBoxPositions.size());
        pokeballBoxes.insert(boxIndex);
    }
    
    // Assign items to all boxes
    for (int i = 0; i < townBoxPositions.size(); i++) {
        if (pokeballBoxes.contains(i)) {
            // This box gets a Poké Ball
            townBoxContents[i] = "Poké Ball";
        } else {
            // This box gets either Ether or Potion randomly
            int itemIndex = rng->bounded(possibleItems.size());
            townBoxContents[i] = possibleItems[itemIndex];
        }
    }
    
    qDebug() << "Assigned items to boxes. Poké Ball boxes:" << pokeballBoxes;
    
    // Mark as initialized
    townBoxesInitialized = true;
}

void Game::setItems(const QMap<QString, int>& items)
{
    inventory = items;
}

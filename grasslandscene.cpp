#include "grasslandscene.h"
#include "game.h"
#include <QDebug>
#include <QGraphicsTextItem>
#include <QFont>
#include <QTextDocument>
#include <QRandomGenerator>
#include <cmath>

// Define constants for the scene size - must match those from Scene class
const int SCENE_WIDTH = 1000;
const int SCENE_HEIGHT = 1000;
const int VIEW_WIDTH = 525;   // View width (smaller than scene)
const int VIEW_HEIGHT = 450;  // View height (smaller than scene)

GrasslandScene::GrasslandScene(Game *game, QGraphicsScene *scene, QObject *parent)
    : Scene(game, scene, parent), backgroundItem(nullptr), playerItem(nullptr),
    townPortalItem(nullptr), bulletinBoardItem(nullptr), currentGrassArea(-1)
{
    // Create update timer
    updateTimer = new QTimer(this);
    connect(updateTimer, &QTimer::timeout, this, &GrasslandScene::updateScene);

    // Create movement timer for continuous walking
    movementTimer = new QTimer(this);
    connect(movementTimer, &QTimer::timeout, this, &GrasslandScene::processMovement);
    movementTimer->setInterval(60);  // smaller number = faster walking speed
}

GrasslandScene::~GrasslandScene()
{
    cleanup();
    if (updateTimer) {
        updateTimer->stop();
        delete updateTimer;
    }
    if (movementTimer) {
        movementTimer->stop();
        delete movementTimer;
    }
}

void GrasslandScene::initialize()
{
    qDebug() << "Initializing Grassland Scene";

    // Set player position to the lower portion of the grassland but above the portal
    playerPos = QPointF(GRASSLAND_WIDTH / 2, GRASSLAND_HEIGHT - 350);
    qDebug() << "Player position set to:" << playerPos.x() << "," << playerPos.y();

    // Create scene elements
    createBackground();
    createBarriers();
    createTallGrassAreas(); // Add tall grass areas
    createPlayer();

    // Reset grass area tracking and spawn Pokémon in all tall grass areas
    grassAreaVisited.clear();
    currentGrassArea = -1;
    wildPokemons.clear();
    
    // Spawn one Pokémon in each tall grass area immediately
    for (int i = 0; i < tallGrassItems.size(); i++) {
        spawnWildPokemon(i);
        grassAreaVisited[i] = true;
    }

    // Set initial camera position to center on player
    updateCamera();

    // Reset movement state to prevent carrying over movement from town scene
    currentPressedKey = 0;
    pressedKeys.clear();
    if (movementTimer && movementTimer->isActive()) {
        movementTimer->stop();
    }

    // Start update timer
    updateTimer->start(16); // 60 FPS
    // Start movement timer for continuous movement
    movementTimer->start(100);
}

void GrasslandScene::cleanup()
{
    qDebug() << "Cleaning up grassland scene";
    
    // Stop timers first
    if (updateTimer && updateTimer->isActive()) {
        updateTimer->stop();
    }
    
    if (movementTimer && movementTimer->isActive()) {
        movementTimer->stop();
    }
    
    // Clear bag display items explicitly
    clearBagDisplayItems();
    
    // Reset movement state
    currentPressedKey = 0;
    pressedKeys.clear();
    
    // Clean up wild Pokémon sprites
    for (WildPokemon& pokemon : wildPokemons) {
        if (pokemon.spriteItem) {
            scene->removeItem(pokemon.spriteItem);
            delete pokemon.spriteItem;
            pokemon.spriteItem = nullptr;
        }
    }
    wildPokemons.clear();
    
    // Reset grass area tracking
    grassAreaVisited.clear();
    currentGrassArea = -1;
    
    // Clean up battle scene
    if (battleSceneItem) {
        scene->removeItem(battleSceneItem);
        delete battleSceneItem;
        battleSceneItem = nullptr;
    }
    inBattleScene = false;
    
    // We shouldn't remove or delete scene items here since the scene is managed by Game
    // Just reset our pointers so we don't try to use them later
    backgroundItem = nullptr;
    playerItem = nullptr;
    barrierItems.clear();
    ledgeItems.clear();
    tallGrassItems.clear();
    bulletinBoardItem = nullptr;
    townPortalItem = nullptr;
    
    // Don't clear the scene, as it's managed by Game
    qDebug() << "Grassland scene cleanup complete";
}

void GrasslandScene::createBackground()
{
    // First create a large black background for the entire scene
    QGraphicsRectItem* blackBackground = scene->addRect(0, 0, SCENE_WIDTH, SCENE_HEIGHT, 
        QPen(Qt::transparent), QBrush(Qt::black));
    blackBackground->setZValue(-1);
    qDebug() << "Black background created with size:" << SCENE_WIDTH << "x" << SCENE_HEIGHT;

    // Load grassland background
    QPixmap background(":/Dataset/Image/scene/GrassLand.png");

    if (background.isNull()) {
        qDebug() << "Grassland background image not found. Check the path.";
        background = QPixmap(GRASSLAND_WIDTH, GRASSLAND_HEIGHT);
        background.fill(QColor(120, 200, 80)); // Green color as fallback
    } else {
        qDebug() << "Grassland background loaded successfully, size:" << background.width() << "x" << background.height();
        
        // Scale the image to fill the grassland area if needed
        if (background.width() != GRASSLAND_WIDTH || background.height() != GRASSLAND_HEIGHT) {
            background = background.scaled(GRASSLAND_WIDTH, GRASSLAND_HEIGHT, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
            qDebug() << "Grassland background scaled to:" << background.width() << "x" << background.height();
        }
    }

    // Position grassland background at (0,0) in scene
    backgroundItem = scene->addPixmap(background);
    backgroundItem->setPos(0, 0);
    backgroundItem->setZValue(0);

    qDebug() << "Grassland background positioned for scrolling view";
    
    // Make sure the scene background is also black
    scene->setBackgroundBrush(Qt::black);
}

void GrasslandScene::createPlayer()
{
    // Load player sprite
    QPixmap playerSprite(":/Dataset/Image/player/player_F.png");

    if (playerSprite.isNull()) {
        qDebug() << "Player sprite not found at :/Dataset/Image/player/player_F.png, creating a placeholder";
        playerSprite = QPixmap(35, 48);
        playerSprite.fill(Qt::red);
    } else {
        qDebug() << "Player sprite loaded successfully";
    }

    playerItem = scene->addPixmap(playerSprite);
    playerItem->setPos(playerPos); // Set initial position
    playerItem->setZValue(3); // Ensure player is on top of other elements
    qDebug() << "Initial player position:" << playerPos.x() << playerPos.y();
}

void GrasslandScene::createBarriers()
{
    // Define barriers for the grassland based on 1000x1667 dimensions
    QVector<QRect> barrierRects = {
        // Boundary barriers to prevent player from walking off the map
        QRect(0, 0, 422, 75),  // Top left barrier
        QRect(570, 0, 458, 75),  // Top right barrier
        QRect(0, 0, 75, GRASSLAND_HEIGHT),  // Left barrier
        QRect(GRASSLAND_WIDTH - 75, 0, 50, GRASSLAND_HEIGHT),  // Right barrier
        QRect(0, GRASSLAND_HEIGHT - 100, 488, 100),  // Bottom left barrier
        QRect(582, GRASSLAND_HEIGHT - 100, 500, 100),  // Bottom right barrier
        
        // Added barriers for the interior areas based on the red lines in the image
        QRect(85,1010,410,105), //Bottom left tree
        QRect(85,600,80,100), //Center left alone tree
        QRect(422,600,240,100), //Beside center left alone tree (3 tree)
        QRect(338,128,80,358),
    };

    // Add barriers (with visible red outlines for debugging)
    for (const QRect &rect : barrierRects) {
        QGraphicsRectItem *barrier = scene->addRect(rect, QPen(Qt::red, 1), QBrush(Qt::transparent));
        barrier->setZValue(5); // Higher zValue to be visible for debugging
        barrierItems.append(barrier);
    }
    
    // Define ledges (one-way barriers, can jump down, can't climb up)
    // Looking at the brown ledges in the image
    QVector<QRect> ledgeRects = {
       
        QRect(82, 231, 246, 20),   
        QRect(420, 231, 244, 20),   
        QRect(82, 440, 248, 20),   
        
        
          
        QRect(170, 646, 240, 20),   
        QRect(85, 851, 77, 20), 
        QRect(213, 851, 160, 20), 
        QRect(469, 851, 650, 20), 

        QRect(GRASSLAND_WIDTH - 253,1105,175,20),
        QRect(82,1315,163,20),
        QRect(417,1315,550,20),
    };
    
    // Add ledges with purple outlines
    for (const QRect &rect : ledgeRects) {
        QGraphicsRectItem *ledge = scene->addRect(rect, QPen(QColor(128, 0, 128), 2), QBrush(QColor(128, 0, 128, 60)));
        ledge->setZValue(4); // Below barriers but still visible
        ledgeItems.append(ledge);
    }
    
    // Create town transition portal (blue box) at position 2 shown in the image
    QRect townPortalRect(GRASSLAND_WIDTH/2 - 50 + 35, GRASSLAND_HEIGHT - 90, 100, 90);
    QGraphicsRectItem *townPortal = scene->addRect(townPortalRect, QPen(Qt::blue, 2), QBrush(QColor(0, 0, 255, 100)));
    townPortal->setZValue(2); // Below player but visible
    townPortalItem = townPortal;
    
    // Create a bulletin board (green box) - fixed position to match the tent/sign
    QRect bulletinBoardRect(373, 1295, 40, 40);
    QGraphicsRectItem *bulletinBoard = scene->addRect(bulletinBoardRect, QPen(Qt::darkGreen, 2), QBrush(QColor(0, 128, 0, 100)));
    bulletinBoard->setZValue(2); // Below player but visible
    bulletinBoardItem = bulletinBoard;
     
    qDebug() << "Created" << barrierItems.size() << "barriers," << ledgeItems.size() << "ledges, 1 town portal, and 1 bulletin board for grassland";
}

void GrasslandScene::createTallGrassAreas()
{
    // Define tall grass areas for wild Pokémon encounters
    QVector<QRect> grassRects = {
        // Tall grass areas from user specifications
        QRect(82, 1337, 374, 168),  // First tall grass area
        QRect(632, 1337, 295, 168), // Second tall grass area
        QRect(500, 1457, 90, 112), // Third tall grass area
        QRect(500, 1006, 256, 210), // Fourth tall grass area
        QRect(428, 251, 483, 207), // Fifth tall grass area
        QRect(662, 533, 244, 210), // Sixth tall grass area
    };

    // Add tall grass areas with yellow outlines
    for (const QRect &rect : grassRects) {
        QGraphicsRectItem *grassArea = scene->addRect(rect, QPen(Qt::yellow, 2), QBrush(QColor(255, 255, 0, 30)));
        grassArea->setZValue(1); // Just above the background
        tallGrassItems.append(grassArea);
    }
    
    qDebug() << "Created" << tallGrassItems.size() << "tall grass areas for wild Pokémon encounters";
}

void GrasslandScene::handleKeyPress(int key)
{
    qDebug() << "Grassland scene key pressed:" << key;

    // Handle Pokémon selection dialogue
    if (isDialogueActive && isPokemonSelectionDialogue) {
        const QVector<Pokemon*>& playerPokemon = game->getPokemon();
        
        // Check for number keys 1-4 (Qt::Key_1 = 49, Qt::Key_4 = 52)
        if (key >= Qt::Key_1 && key <= Qt::Key_4) {
            int selectedIndex = key - Qt::Key_1; // Convert key to 0-based index
            if (selectedIndex < playerPokemon.size()) {
                // Valid selection
                closeDialogue();
                isPokemonSelectionDialogue = false;
                selectedBattleOption = FIGHT; // Start with FIGHT selected
                showBattleScene();
                return;
            }
        }
        // Check for ESC key to run away
        else if (key == Qt::Key_Escape) {
            closeDialogue();
            isPokemonSelectionDialogue = false;
            exitBattleScene();
            return;
        }
        return; // Ignore other keys during Pokémon selection
    }

    // If in battle scene, handle battle menu navigation
    if (inBattleScene) {
        // If in move selection, handle move choice
        if (isMoveSelectionActive) {
            if (key == Qt::Key_B || key == Qt::Key_Escape) {
                // Return to battle menu
                isMoveSelectionActive = false;
                showBattleScene();
                return;
            }

            // Get player's active Pokémon
            const QVector<Pokemon*>& playerPokemon = game->getPokemon();
            if (!playerPokemon.isEmpty()) {
                Pokemon* activePokemon = playerPokemon.first();
                const QVector<Pokemon::Move>& moves = activePokemon->getMoves();

                // Handle move selection (keys 1-2 for moves, C for Do Nothing)
                if (key >= Qt::Key_1 && key <= Qt::Key_2) {
                    int moveIndex = key - Qt::Key_1;
                    if (moveIndex < moves.size()) {
                        // Move selected - handle the move
                        handleMoveSelection(moveIndex);
                        isMoveSelectionActive = false;
                        return;
                    }
                } else if (key == Qt::Key_C) {
                    // Do Nothing selected
                    handleMoveSelection(-1); // Use -1 to indicate "Do Nothing"
                    isMoveSelectionActive = false;
                    return;
                }
            }
            return; // Ignore other keys during move selection
        }

        // If in bag view, handle item selection
        if (isBattleBagOpen) {
            QMap<QString, int> inventory = game->getItems();
            
            if (key == Qt::Key_B || key == Qt::Key_Escape) {
                // Close bag and return to battle menu
                isBattleBagOpen = false;
                showBattleScene();
                return;
            }
            
            // Handle item selection
            if (key >= Qt::Key_1 && key <= Qt::Key_3) {
                int itemIndex = key - Qt::Key_1 + 1; // Convert to 1-based index
                handleBagSelection(itemIndex);
                return;
            }
            return; // Ignore other keys while bag is open
        }

        // Remember the previous selection to update styling
        BattleOption prevSelection = selectedBattleOption;
        
        // Handle arrow key navigation in the battle menu
        switch (key) {
            case Qt::Key_Up:
                if (selectedBattleOption == POKEMON) selectedBattleOption = FIGHT;
                else if (selectedBattleOption == RUN) selectedBattleOption = BAG;
                break;
                
            case Qt::Key_Down:
                if (selectedBattleOption == FIGHT) selectedBattleOption = POKEMON;
                else if (selectedBattleOption == BAG) selectedBattleOption = RUN;
                break;
                
            case Qt::Key_Left:
                if (selectedBattleOption == BAG) selectedBattleOption = FIGHT;
                else if (selectedBattleOption == RUN) selectedBattleOption = POKEMON;
                break;
                
            case Qt::Key_Right:
                if (selectedBattleOption == FIGHT) selectedBattleOption = BAG;
                else if (selectedBattleOption == POKEMON) selectedBattleOption = RUN;
                break;
                
            case Qt::Key_A:
            case Qt::Key_Return:
            case Qt::Key_Enter:
                if (selectedBattleOption == RUN || key == Qt::Key_A) {
                    exitBattleScene();
                    return;
                } else if (selectedBattleOption == BAG) {
                    showBattleBag();
                    return;
                } else if (selectedBattleOption == FIGHT) {
                    showMoveSelection();
                    return;
                }
                break;
        }
        
        // Update the menu if selection changed
        if (prevSelection != selectedBattleOption) {
            qDebug() << "Battle menu selection changed from" << prevSelection << "to" << selectedBattleOption;
            showBattleScene();
        }
        
        return;
    }

    // If bag is open, only allow B key to close it
    if (isBagOpen) {
        if (key == Qt::Key_B) {
            toggleBag();
        }
        return; // Block all other key presses while bag is open
    }

    // If dialogue is active, only allow A to advance
    if (isDialogueActive) {
        if (key == Qt::Key_A) {
            handleDialogue();
        }
        return;
    }

    // Add key to pressed keys set for movement
    pressedKeys.insert(key);
    qDebug() << "Key pressed: " << key;

    // For arrow keys, set as current pressed key for continuous movement
    // and also take a small step immediately for responsive feel
    if (key == Qt::Key_Up || key == Qt::Key_Down || key == Qt::Key_Left || key == Qt::Key_Right) {
        QPointF prevPos = playerPos;
        bool moved = false;
        
        // Take immediate step (5 pixels)
        if (key == Qt::Key_Up) {
            playerDirection = "B";
            playerPos.setY(playerPos.y() - 5);
            moved = true;
        } else if (key == Qt::Key_Down) {
            playerDirection = "F";
            playerPos.setY(playerPos.y() + 5);
            moved = true;
        } else if (key == Qt::Key_Left) {
            playerDirection = "L";
            playerPos.setX(playerPos.x() - 5);
            moved = true;
        } else if (key == Qt::Key_Right) {
            playerDirection = "R";
            playerPos.setX(playerPos.x() + 5);
            moved = true;
        }
        
        if (moved) {
            // Boundary checks
            if (playerPos.x() < 0) {
                playerPos.setX(0);
            } else if (playerPos.x() > GRASSLAND_WIDTH - 25) {
                playerPos.setX(GRASSLAND_WIDTH - 25);
            }

            if (playerPos.y() < 0) {
                playerPos.setY(0);
            } else if (playerPos.y() > GRASSLAND_HEIGHT - 48) {
                playerPos.setY(GRASSLAND_HEIGHT - 48);
            }
            
            // Collision check
            QRectF playerRect(playerPos.x() + 5, playerPos.y() + 30, 25, 18);
            bool collision = false;
            
            // Check regular barrier collisions
            for (const QGraphicsRectItem* barrier : barrierItems) {
                QRectF barrierRect = barrier->rect();
                if (playerRect.intersects(barrierRect)) {
                    collision = true;
                    break;
                }
            }
            
            // Check ledge collisions - only when moving upward
            if (!collision && key == Qt::Key_Up) {
                // Check if player is trying to climb up a ledge (not allowed)
                for (const QGraphicsRectItem* ledge : ledgeItems) {
                    QRectF ledgeRect = ledge->rect();
                    
                    // Improved logic for immediate movement
                    QRectF oldPlayerRect(prevPos.x() + 5, prevPos.y() + 30, 25, 18); // Previous position
                    
                    // Check same conditions as in processMovement:
                    // 1. Player's previous position was below the ledge
                    // 2. Player is trying to cross the ledge vertically
                    // 3. Player is horizontally aligned with the ledge
                    bool wasBelow = oldPlayerRect.top() > ledgeRect.top();
                    bool isCrossing = playerRect.top() <= ledgeRect.bottom() && playerRect.top() > ledgeRect.top();
                    bool isAligned = playerRect.right() >= ledgeRect.left() && playerRect.left() <= ledgeRect.right();
                    
                    if (wasBelow && isCrossing && isAligned) {
                        qDebug() << "LEDGE BLOCKED: Player blocked from climbing ledge during immediate movement";
                        collision = true;
                        break;
                    }
                }
            }
            
            // Handle jumping down ledges for immediate movement
            bool jumpingDownLedge = false;
            if (key == Qt::Key_Down) {
                jumpingDownLedge = isPlayerJumpingDownLedge(playerPos);
            }
            
            if (collision && !jumpingDownLedge) {
                playerPos = prevPos;
            } else {
                // Update position and sprite
                if (playerItem) {
                    playerItem->setPos(playerPos);
                    qDebug() << "Immediate step: player position updated to:" << playerPos;
                }
                walkFrame = (walkFrame + 1) % 3;
                updatePlayerSprite();
                updateCamera();
            }
        }
        
        // Set current key for continuous movement
        currentPressedKey = key;
        // Start the movement timer if it's not already running
        if (!movementTimer->isActive()) {
            movementTimer->start(100);
        }
    }

    // Check for B key to open bag
    if (key == Qt::Key_B) {
        toggleBag();
        return;
    }

    // Check for A key to interact with objects
    if (key == Qt::Key_A) {
        // Check if player is near the bulletin board
        if (isPlayerNearBulletinBoard()) {
            showDialogue("GRASSLAND BULLETIN: Wild Pokémon can be found in the tall grass. Be careful and always carry your Pokémon with you!");
            return;
        }
        
        qDebug() << "Player pressed A but no interactive objects are nearby";
    }
}

void GrasslandScene::handleKeyRelease(int key)
{
    // Remove key from pressed keys set
    pressedKeys.remove(key);
    
    // If the released key was the current movement key, reset it
    if (key == currentPressedKey) {
        currentPressedKey = 0;
        
        // Stop the movement timer if no arrow keys are pressed
        if (!pressedKeys.contains(Qt::Key_Up) && 
            !pressedKeys.contains(Qt::Key_Down) && 
            !pressedKeys.contains(Qt::Key_Left) && 
            !pressedKeys.contains(Qt::Key_Right)) {
            movementTimer->stop();
        }
    }
}

void GrasslandScene::processMovement()
{
    // Do nothing if no key is pressed or dialogue/bag is open
    if (currentPressedKey == 0 || isDialogueActive || isBagOpen) {
        return;
    }
    
    QPointF prevPos = playerPos;
    bool moved = false;

    // Speed increases after holding the key for some time
    static int moveSteps = 0;
    int moveSpeed = 8; // Base speed
    
    // Increase speed by 20% after a few steps
    if (moveSteps > 3) {
        moveSpeed = 10; // Increased by 20% from base
    }
    moveSteps++;

    // Movement based on currently pressed key
    if (currentPressedKey == Qt::Key_Up) {
        playerPos.setY(playerPos.y() - moveSpeed);
        playerDirection = "B";
        moved = true;
    } else if (currentPressedKey == Qt::Key_Down) {
        playerPos.setY(playerPos.y() + moveSpeed);
        playerDirection = "F";
        moved = true;
    } else if (currentPressedKey == Qt::Key_Left) {
        playerPos.setX(playerPos.x() - moveSpeed);
        playerDirection = "L";
        moved = true;
    } else if (currentPressedKey == Qt::Key_Right) {
        playerPos.setX(playerPos.x() + moveSpeed);
        playerDirection = "R";
        moved = true;
    }

    if (moved) {
        // Boundary checking - don't allow player to go outside grassland area
        if (playerPos.x() < 0) {
            playerPos.setX(0);
        } else if (playerPos.x() > GRASSLAND_WIDTH - 25) {
            playerPos.setX(GRASSLAND_WIDTH - 25);
        }

        if (playerPos.y() < 0) {
            playerPos.setY(0);
        } else if (playerPos.y() > GRASSLAND_HEIGHT - 48) {
            playerPos.setY(GRASSLAND_HEIGHT - 48);
        }

        // Check collision with barriers using a smaller hitbox at player's feet
        QRectF playerRect(playerPos.x() + 5, playerPos.y() + 30, 25, 18);
        bool collision = false;
        
        // Check regular barrier collisions
        for (const QGraphicsRectItem* barrier : barrierItems) {
            QRectF barrierRect = barrier->rect();
            if (playerRect.intersects(barrierRect)) {
                collision = true;
                break;
            }
        }
        
        // Check ledge collisions - only if moving upward
        if (!collision && currentPressedKey == Qt::Key_Up) {
            for (const QGraphicsRectItem* ledge : ledgeItems) {
                QRectF ledgeRect = ledge->rect();
                
                // Improve the calculation of player's feet position in relation to the ledge
                QRectF oldPlayerRect(prevPos.x() + 5, prevPos.y() + 30, 25, 18); // Previous position
                
                // Check if:
                // 1. Player's previous position was below the ledge
                // 2. Player is trying to cross the ledge vertically
                // 3. Player is horizontally aligned with the ledge
                bool wasBelow = oldPlayerRect.top() > ledgeRect.top();
                bool isCrossing = playerRect.top() <= ledgeRect.bottom() && playerRect.top() > ledgeRect.top();
                bool isAligned = playerRect.right() >= ledgeRect.left() && playerRect.left() <= ledgeRect.right();
                
                if (wasBelow && isCrossing && isAligned) {
                    qDebug() << "LEDGE BLOCKED: Player blocked from climbing ledge at" << ledgeRect;
                    collision = true;
                    break;
                }
            }
        }
        
        // Allow jumping down ledges
        bool jumpingDownLedge = isPlayerJumpingDownLedge(playerPos);

        if (collision && !jumpingDownLedge) {
            playerPos = prevPos;
        } else {
            // Update walk frame only if we actually moved
            walkFrame = (walkFrame + 1) % 3;
            
            // Update player visual position
            if (playerItem) {
                playerItem->setPos(playerPos);
                qDebug() << "Continuous movement: player position updated to:" << playerPos;
            }
            
            // Update player sprite based on direction and walk frame
            updatePlayerSprite();
            
            // Update camera to follow player
            updateCamera();
        }
    }
}

void GrasslandScene::updateScene()
{
    // Skip updates if in battle scene
    if (inBattleScene) {
        return;
    }
    
    // Call our frame update logic
    update();
    
    // Check if player is on the town portal to return to town
    if (isPlayerNearTownPortal()) {
        qDebug() << "Player is on town portal, changing scene to town";
        
        // Clean up grassland scene
        cleanup();
        
        // Change scene back to town
        game->changeScene(GameState::TOWN);
    }
}

void GrasslandScene::updatePlayerSprite()
{
    QString basePath = ":/Dataset/Image/player/player_";
    QString spritePath;

    // If not moving or first frame, use the base sprite for that direction
    if (walkFrame == 0) {
        spritePath = basePath + playerDirection + ".png";
    } else {
        // Use the walking animation frames (W1 or W2)
        spritePath = basePath + playerDirection + "W" + QString::number(walkFrame) + ".png";
    }
    
    QPixmap playerSprite(spritePath);
    
    if (playerSprite.isNull()) {
        qDebug() << "Failed to load sprite:" << spritePath;
        // Fallback to default sprite if animation frame not found
        if (playerDirection == "F") {
            playerSprite = QPixmap(":/Dataset/Image/player/player_F.png");
        } else {
            // If even the fallback fails, create a colored rectangle
            playerSprite = QPixmap(35, 48);
            playerSprite.fill(Qt::red);
        }
    }
    
    if (playerItem) {
        playerItem->setPixmap(playerSprite);
    }
}

void GrasslandScene::updateCamera()
{
    // Make sure playerItem exists
    if (!playerItem) return;
    
    // Get player position (center of player)
    QPointF playerCenter = playerPos + QPointF(17.5, 24); // Center of the player sprite
    
    // Calculate desired camera position (centered on player)
    QPointF targetCameraPos;
    targetCameraPos.setX(playerCenter.x() - VIEW_WIDTH / 2);
    targetCameraPos.setY(playerCenter.y() - VIEW_HEIGHT / 2);
    
    // Calculate boundary constraints
    float minCameraX = 0; // Left boundary
    float maxCameraX = GRASSLAND_WIDTH - VIEW_WIDTH; // Right boundary
    float minCameraY = 0; // Top boundary
    float maxCameraY = GRASSLAND_HEIGHT - VIEW_HEIGHT; // Bottom boundary
    
    // Apply constraints - prevent camera from going outside grassland boundaries
    if (targetCameraPos.x() < minCameraX) targetCameraPos.setX(minCameraX);
    if (targetCameraPos.y() < minCameraY) targetCameraPos.setY(minCameraY);
    if (targetCameraPos.x() > maxCameraX) targetCameraPos.setX(maxCameraX);
    if (targetCameraPos.y() > maxCameraY) targetCameraPos.setY(maxCameraY);
    
    // Update camera position
    cameraPos = targetCameraPos;
    
    // Update the view - this makes the camera follow the player
    scene->setSceneRect(cameraPos.x(), cameraPos.y(), VIEW_WIDTH, VIEW_HEIGHT);
    
    // Debug info
    qDebug() << "Camera at:" << cameraPos << "Player at:" << playerPos;
    
    // Update dialogue box position if active
    if (isDialogueActive && dialogBoxItem) {
        dialogBoxItem->setPos(cameraPos.x() + 10, cameraPos.y() + VIEW_HEIGHT - 100);
        if (dialogTextItem) {
            dialogTextItem->setPos(cameraPos.x() + 20, cameraPos.y() + VIEW_HEIGHT - 90);
        }
    }
    
    // Update bag position if open
    if (isBagOpen && bagBackgroundItem) {
        // Center the bag in the current view
        QPixmap bagPixmap = bagBackgroundItem->pixmap();
        float bagX = cameraPos.x() + (VIEW_WIDTH - bagPixmap.width()) / 2;
        float bagY = cameraPos.y() + (VIEW_HEIGHT - bagPixmap.height()) / 2;
        bagBackgroundItem->setPos(bagX, bagY);
        
        // Update all bag items to new position
        updateBagDisplay();
    }
}

bool GrasslandScene::checkCollision()
{
    // Boundary checking
    if (playerPos.x() < 0 || playerPos.x() > GRASSLAND_WIDTH - 35 ||
        playerPos.y() < 0 || playerPos.y() > GRASSLAND_HEIGHT - 48) {
        return true;
    }

    // Check collision with barriers - use smaller hitbox at player's feet
    QRectF playerRect(playerPos.x() + 5, playerPos.y() + 30, 25, 18);
    
    for (const QGraphicsRectItem* barrier : barrierItems) {
        if (playerRect.intersects(barrier->rect())) {
            return true;
        }
    }

    return false;
}

void GrasslandScene::updatePlayerPosition()
{
    if (playerItem) {
        playerItem->setPos(playerPos);
        // Camera will follow player
        updateCamera();
    }
}

void GrasslandScene::toggleBag()
{
    if (isBagOpen) {
        // Close the bag
        clearBagDisplayItems();
        
        isBagOpen = false;
        qDebug() << "Bag closed";
    } else {
        // Open the bag
        isBagOpen = true;
        qDebug() << "Bag opened";
        
        // Update the display to show the Pokémon in the bag
        updateBagDisplay();
    }
}

void GrasslandScene::clearBagDisplayItems()
{
    // Clear Pokémon sprites
    for (auto sprite : bagPokemonSprites) {
        if (sprite) {
            scene->removeItem(sprite);
            delete sprite;
        }
    }
    bagPokemonSprites.clear();

    // Clear Pokémon name texts
    for (auto text : bagPokemonNames) {
        if (text) {
            scene->removeItem(text);
            delete text;
        }
    }
    bagPokemonNames.clear();

    // Clear other bag-related items (like rectangles)
    for (auto item : bagSlotRects) {
        if (item) {
            scene->removeItem(item);
            delete item;
        }
    }
    bagSlotRects.clear();
    
    // Clear bag background
    if (bagBackgroundItem) {
        scene->removeItem(bagBackgroundItem);
        delete bagBackgroundItem;
        bagBackgroundItem = nullptr;
    }
    
    qDebug() << "Cleared bag display items.";
}

void GrasslandScene::updateBagDisplay()
{
    // Always clear previous items before drawing new ones
    clearBagDisplayItems();

    // If bag is not open, do nothing further
    if (!isBagOpen) {
        return;
    }

    // Create bag background image
    QPixmap bagPixmap(":/Dataset/Image/bag.png");
    if (bagPixmap.isNull()) {
        qDebug() << "Failed to load bag image from :/Dataset/Image/bag.png";
        return;
    }
    
    // Scale bag image to be 25% bigger
    QSize originalSize = bagPixmap.size();
    QSize newSize(originalSize.width() * 1.25, originalSize.height() * 1.25);
    bagPixmap = bagPixmap.scaled(newSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    
    // Position in center of view
    float bagX = cameraPos.x() + (VIEW_WIDTH - bagPixmap.width()) / 2;
    float bagY = cameraPos.y() + (VIEW_HEIGHT - bagPixmap.height()) / 2;
    
    bagBackgroundItem = scene->addPixmap(bagPixmap);
    bagBackgroundItem->setPos(bagX, bagY);
    bagBackgroundItem->setZValue(100);
    
    qDebug() << "Added bag background at" << bagX << "," << bagY << "with size" << bagPixmap.size();

    // Add row.png image on top of the bag
    QPixmap rowPixmap(":/Dataset/Image/row.png");
    if (!rowPixmap.isNull()) {
        // Scale the row image to match the width of the bag
        rowPixmap = rowPixmap.scaled(bagPixmap.width(), rowPixmap.height(), 
                                     Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
        
        // Position at the top of the bag, but higher up to not take space from the first row
        QGraphicsPixmapItem* rowItem = scene->addPixmap(rowPixmap);
        rowItem->setPos(bagX, bagY - rowPixmap.height() * 0.75); // Move up by 75% of its height
        rowItem->setZValue(101); // Above the bag but below the Pokémon
        bagPokemonSprites.append(rowItem); // Add to sprites so it gets cleaned up when bag closes
        
        qDebug() << "Added row image on top of bag";
        
        // Now add the items count to the row
        // Get player's inventory
        QMap<QString, int> inventory = game->getItems();
        
        // Define item icons and their positions in the row
        struct ItemInfo {
            QString name;
            QString iconPath;
            float xOffset;  // Horizontal position in the row
        };
        
        std::vector<ItemInfo> items = {
            {"Poké Ball", ":/Dataset/Image/icon/Pokeball_bag.png", 0.15f},   // Left position
            {"Potion", ":/Dataset/Image/icon/Potion_bag.png", 0.5f},        // Middle position
            {"Ether", ":/Dataset/Image/icon/Ether_bag.png", 0.85f}           // Right position
        };
        
        // Add each item with its count
        for (const auto& item : items) {
            int count = inventory.value(item.name, 0);
            
            // Skip if count is 0
            if (count == 0) continue;
            
            // Enforce the maximum of 3 Poké Balls
            if (item.name == "Poké Ball" && count > 3) {
                count = 3;
            }
            
            // Load item icon
            QPixmap itemIcon(item.iconPath);
            if (!itemIcon.isNull()) {
                // Scale icon to appropriate size (25x25 pixels) - slightly smaller
                itemIcon = itemIcon.scaled(25, 25, Qt::KeepAspectRatio, Qt::SmoothTransformation);
                
                // Calculate position in the row to ensure it stays within boundaries
                float rowWidth = rowPixmap.width();
                // Use margins at edges of the row
                float effectiveRowWidth = rowWidth * 0.85; // Reduced from 0.9 to 0.85
                float startX = bagX + (rowWidth - effectiveRowWidth) / 2 - 8; // Add 8px left offset
                
                // Position icon within the row's safe area
                float iconX = startX + (effectiveRowWidth * item.xOffset) - (itemIcon.width() / 2);
                float iconY = bagY - rowPixmap.height() / 2 - itemIcon.height() / 2 + 6; // Add 6px down offset
                
                // Add icon to scene
                QGraphicsPixmapItem* iconItem = scene->addPixmap(itemIcon);
                iconItem->setPos(iconX, iconY);
                iconItem->setZValue(102);
                bagPokemonSprites.append(iconItem);
                
                // Add count text ("x1", "x2", etc.)
                QFont countFont("Arial", 10, QFont::Bold);
                QGraphicsTextItem* countText = scene->addText("x" + QString::number(count), countFont);
                countText->setDefaultTextColor(Qt::black);
                countText->setZValue(102);
                // Position text closer to icon to save space
                countText->setPos(iconX + itemIcon.width(), iconY + 2);
                bagPokemonNames.append(countText);
                
                qDebug() << "Added item" << item.name << "with count" << count << "at position" << iconX << "," << iconY;
            } else {
                qDebug() << "Failed to load item icon from" << item.iconPath;
            }
        }
    } else {
        qDebug() << "Failed to load row image from :/Dataset/Image/row.png";
    }

    // Get the player's Pokémon collection
    const QVector<Pokemon*>& playerPokemon = game->getPokemon();
    if (playerPokemon.isEmpty()) {
        qDebug() << "No Pokémon in player's collection to display";
        return; // Return here but after drawing the bag background and row
    }

    qDebug() << "Player has" << playerPokemon.size() << "Pokémon:";
    for (int i = 0; i < playerPokemon.size(); i++) {
        qDebug() << i << ":" << playerPokemon[i]->getName() << "with image path:" << playerPokemon[i]->getImagePath();
    }

    // Display each Pokémon in the bag - the same as in town scene
    const int ROW_HEIGHT = 40;
    const int ROW_SPACING = 15;
    
    // Start at a higher position for first row - back to original position
    float startY = bagY + 5; // Original position, no longer need to increase for row.png
    
    // Calculate the width of the bag content area (80% of bag width to leave margins)
    float contentWidth = bagPixmap.width() * 0.8;
    float contentX = bagX + (bagPixmap.width() - contentWidth) / 2;
    
    for (int i = 0; i < playerPokemon.size() && i < 4; i++) {
        Pokemon* pokemon = playerPokemon[i];
        
        // Load the Pokémon image
        QPixmap pokemonImage(pokemon->getImagePath());
        if (pokemonImage.isNull()) {
            qDebug() << "Failed to load Pokémon image for" << pokemon->getName() << "at" << pokemon->getImagePath();
            continue;
        }
        
        // Scale the image to fit the row height
        pokemonImage = pokemonImage.scaled(ROW_HEIGHT, ROW_HEIGHT, 
                                          Qt::KeepAspectRatio, 
                                          Qt::SmoothTransformation);
        
        // Calculate row position with even spacing
        float rowY = startY + i * (ROW_HEIGHT + ROW_SPACING);
        
        // Create the Pokémon name text first (on the left)
        QFont nameFont("Arial", 12, QFont::Bold);
        QGraphicsTextItem* nameText = scene->addText(pokemon->getName(), nameFont);
        nameText->setDefaultTextColor(Qt::black);
        nameText->setZValue(102); // Above both bag and row
        
        // Position name on the left side of the row
        float textX = contentX;
        float textY = rowY + (ROW_HEIGHT - nameText->boundingRect().height()) / 2;
        nameText->setPos(textX, textY);
        bagPokemonNames.append(nameText);
        
        // Add the Pokémon sprite on the right
        QGraphicsPixmapItem* pokemonSprite = scene->addPixmap(pokemonImage);
        
        // Position image on the right side of the row
        float spriteX = contentX + contentWidth - pokemonImage.width();
        float spriteY = rowY + (ROW_HEIGHT - pokemonImage.height()) / 2;
        
        pokemonSprite->setPos(spriteX, spriteY);
        pokemonSprite->setZValue(102); // Above both bag and row
        bagPokemonSprites.append(pokemonSprite);
        
        qDebug() << "Added" << pokemon->getName() << "to bag at row" << i 
                 << "text at:" << textX << "," << textY
                 << "sprite at:" << spriteX << "," << spriteY;
    }

    qDebug() << "Bag display updated with" << bagPokemonSprites.size() << "Pokémon";
}

void GrasslandScene::showDialogueBox(const QString &text)
{
    // Remove any existing dialogue box and text
    if (dialogBoxItem) {
        scene->removeItem(dialogBoxItem);
        delete dialogBoxItem;
        dialogBoxItem = nullptr;
    }
    
    if (dialogTextItem) {
        scene->removeItem(dialogTextItem);
        delete dialogTextItem;
        dialogTextItem = nullptr;
    }
    
    // Create the dialogue box using the image
    QPixmap dialogBox(":/Dataset/Image/dialog.png");
    if (dialogBox.isNull()) {
        qDebug() << "Dialog box image not found, creating a fallback rectangle";
        dialogBoxItem = scene->addRect(0, 0, VIEW_WIDTH - 20, 120, QPen(Qt::black), QBrush(QColor(255, 255, 255, 200)));
    } else {
        // Scale dialog box to ensure it can fit more text
        dialogBox = dialogBox.scaled(VIEW_WIDTH - 20, 120, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
        dialogBoxItem = scene->addPixmap(dialogBox);
    }
    
    // Position the dialogue box at the bottom of the screen
    dialogBoxItem->setPos(cameraPos.x() + 10, cameraPos.y() + VIEW_HEIGHT - dialogBox.height() - 10);
    dialogBoxItem->setZValue(90); // Above most elements
    
    // Create text with appropriate font
    QFont dialogFont("Arial", 11);
    dialogTextItem = scene->addText(text, dialogFont);
    dialogTextItem->setDefaultTextColor(Qt::black);
    
    // Position the text inside the dialogue box with some padding
    float textX = cameraPos.x() + 25;
    float textY = cameraPos.y() + VIEW_HEIGHT - dialogBox.height() + 5;
    dialogTextItem->setPos(textX, textY);
    dialogTextItem->setZValue(91); // Above dialogue box
    
    // Set text width to wrap long lines
    QTextDocument* doc = dialogTextItem->document();
    doc->setTextWidth(VIEW_WIDTH - 50); // Allow text to wrap with more room
    
    // Set dialogue as active
    isDialogueActive = true;
}

void GrasslandScene::showDialogue(const QString &text)
{
    // Use the existing showDialogueBox method
    showDialogueBox(text);
}

void GrasslandScene::handleDialogue()
{
    // For now, just close the dialogue when A is pressed
    closeDialogue();
}

void GrasslandScene::closeDialogue()
{
    // Remove dialogue box and text
    if (dialogBoxItem) {
        scene->removeItem(dialogBoxItem);
        delete dialogBoxItem;
        dialogBoxItem = nullptr;
    }
    
    if (dialogTextItem) {
        scene->removeItem(dialogTextItem);
        delete dialogTextItem;
        dialogTextItem = nullptr;
    }
    
    // Reset dialogue state
    isDialogueActive = false;
    currentDialogueState = 0;
}

bool GrasslandScene::isPlayerNearTownPortal() const
{
    // Get player's feet position (collision box)
    QRectF playerFeet(playerPos.x() + 5, playerPos.y() + 30, 25, 18);
    
    if (!townPortalItem) {
        qDebug() << "Town portal item is null!";
        return false;
    }
    
    QRectF portalRect = townPortalItem->rect();
    
    // Check if player's feet area intersects with the portal
    bool isOnPortal = playerFeet.intersects(portalRect);
    
    // Removed debug output to reduce console spam
    
    // Player must be directly on the portal to transport
    return isOnPortal;
}

bool GrasslandScene::isPlayerNearBulletinBoard() const
{
    // Get player's feet position (collision box)
    QRectF playerFeet(playerPos.x() + 5, playerPos.y() + 30, 25, 18);
    
    if (!bulletinBoardItem) {
        qDebug() << "Bulletin board item is null!";
        return false;
    }
    
    QRectF boardRect = bulletinBoardItem->rect();
    
    // Create an expanded detection area around the bulletin board
    QRectF expandedArea = boardRect.adjusted(-20, -20, 20, 20);
    
    // Check if player's feet area intersects with the expanded detection area
    bool isNearBoard = playerFeet.intersects(expandedArea);
    
    // Debug output
    qDebug() << "Bulletin board at:" << boardRect
             << "player feet at:" << playerFeet
             << "isNearBoard:" << isNearBoard;
    
    return isNearBoard;
}

bool GrasslandScene::isPlayerJumpingDownLedge(const QPointF& newPos) const
{
    // Get player's current position and the new position after movement
    QRectF currentPlayerRect(playerPos.x() + 5, playerPos.y() + 30, 25, 18); // Current feet position
    QRectF newPlayerRect(newPos.x() + 5, newPos.y() + 30, 25, 18); // New feet position
    
    // Only check for ledge jumping if moving downward
    if (newPos.y() <= playerPos.y()) {
        return false; // Not moving down, so not jumping a ledge
    }
    
    // Check each ledge
    for (const QGraphicsRectItem* ledge : ledgeItems) {
        QRectF ledgeRect = ledge->rect();
        
        // Check if:
        // 1. Current position is above or on the ledge
        // 2. New position is below the ledge
        // 3. Player is horizontally within the ledge width
        
        bool currentlyAboveLedge = currentPlayerRect.bottom() <= ledgeRect.top() + 2; // +2 for a bit of tolerance
        bool movingBelowLedge = newPlayerRect.top() > ledgeRect.bottom();
        bool horizontallyAligned = (newPlayerRect.left() <= ledgeRect.right() && 
                                   newPlayerRect.right() >= ledgeRect.left());
        
        if (currentlyAboveLedge && horizontallyAligned && movingBelowLedge) {
            // Allow jumping down
            qDebug() << "Player jumping down ledge at" << ledgeRect;
            return true;
        }
    }
    
    return false;
}

void GrasslandScene::updateBarrierVisibility()
{
    // Update visibility of barrier outlines based on debug mode
    for (QGraphicsRectItem* barrier : barrierItems) {
        if (barrier) {
            if (debugMode) {
                // In debug mode, make barriers visible with red outlines
                barrier->setPen(QPen(Qt::red, 2));
                barrier->setBrush(QBrush(QColor(255, 0, 0, 40))); // Semi-transparent red
            } else {
                // In normal mode, make barriers invisible
                barrier->setPen(QPen(Qt::transparent));
                barrier->setBrush(QBrush(Qt::transparent));
            }
        }
    }
    
    // Update ledge visibility
    for (QGraphicsRectItem* ledge : ledgeItems) {
        if (ledge) {
            if (debugMode) {
                // In debug mode, make ledges more visible with purple outlines
                ledge->setPen(QPen(QColor(128, 0, 128), 2));
                ledge->setBrush(QBrush(QColor(128, 0, 128, 80))); // More visible purple
            } else {
                // In normal mode, make ledges subtly visible
                ledge->setPen(QPen(QColor(128, 0, 128), 1));
                ledge->setBrush(QBrush(QColor(128, 0, 128, 40))); // Less visible purple
            }
        }
    }
    
    // Update tall grass visibility
    for (QGraphicsRectItem* grass : tallGrassItems) {
        if (grass) {
            if (debugMode) {
                // In debug mode, make tall grass more visible with yellow outlines
                grass->setPen(QPen(Qt::yellow, 2));
                grass->setBrush(QBrush(QColor(255, 255, 0, 60))); // More visible yellow
            } else {
                // In normal mode, make tall grass barely visible
                grass->setPen(QPen(Qt::transparent));
                grass->setBrush(QBrush(QColor(255, 255, 0, 15))); // Very subtle yellow
            }
        }
    }
    
    // Update town portal visibility
    if (townPortalItem) {
        if (debugMode) {
            // In debug mode, make portal more visible with blue outline
            townPortalItem->setPen(QPen(Qt::blue, 3));
            townPortalItem->setBrush(QBrush(QColor(0, 0, 255, 80))); // More visible blue
        } else {
            // In normal mode, use standard subtle blue
            townPortalItem->setPen(QPen(Qt::blue, 2));
            townPortalItem->setBrush(QBrush(QColor(0, 0, 255, 100)));
        }
    }
    
    // Update bulletin board visibility
    if (bulletinBoardItem) {
        if (debugMode) {
            // In debug mode, make bulletin board more visible with green outline
            bulletinBoardItem->setPen(QPen(Qt::green, 3));
            bulletinBoardItem->setBrush(QBrush(QColor(0, 255, 0, 80))); // More visible green
            
            // Add a label to the bulletin board
            QPointF pos = bulletinBoardItem->rect().topLeft();
            QGraphicsTextItem* label = scene->addText("Bulletin Board");
            label->setPos(pos.x(), pos.y() - 20);
            label->setDefaultTextColor(Qt::green);
            label->setZValue(100);
        } else {
            // In normal mode, use standard subtle green
            bulletinBoardItem->setPen(QPen(Qt::darkGreen, 2));
            bulletinBoardItem->setBrush(QBrush(QColor(0, 128, 0, 100)));
            
            // Remove any existing labels
            // Note: This is simplified and might need more robust handling
            for (QGraphicsItem* item : scene->items()) {
                QGraphicsTextItem* textItem = dynamic_cast<QGraphicsTextItem*>(item);
                if (textItem && textItem->toPlainText() == "Bulletin Board") {
                    scene->removeItem(textItem);
                    delete textItem;
                }
            }
        }
    }
}

void GrasslandScene::update()
{
    // Skip updates if dialogue or bag is open or in battle
    if (isDialogueActive || isBagOpen || inBattleScene) {
        return;
    }
    
    // If player steps into a tall grass area
    int areaIndex;
    bool inGrass = isPlayerInGrassArea(&areaIndex);
    
    // Handle entering/exiting grass areas
    if (inGrass) {
        if (currentGrassArea != areaIndex) {
            // Player entered a new grass area
            qDebug() << "Player moved from grass area" << currentGrassArea << "to" << areaIndex;
            currentGrassArea = areaIndex;
            
            // Check if there's an active Pokémon in this area
            bool hasPokemonInArea = false;
            for (const WildPokemon& pokemon : wildPokemons) {
                if (!pokemon.encountered && pokemon.spriteItem) {
                    QRectF grassRect = tallGrassItems[areaIndex]->rect();
                    if (grassRect.contains(pokemon.position)) {
                        hasPokemonInArea = true;
                        break;
                    }
                }
            }
            
            // If no active Pokémon in this area, spawn one
            if (!hasPokemonInArea) {
                qDebug() << "No active Pokémon in grass area" << areaIndex << "- spawning new wild Pokémon";
                spawnWildPokemon(areaIndex);
                grassAreaVisited[areaIndex] = true;
            }
        }
        
        // Check for collisions with wild Pokémon
        checkWildPokemonCollision();
    } else if (currentGrassArea != -1) {
        // Player exited a grass area
        qDebug() << "Player exited grass area" << currentGrassArea;
        currentGrassArea = -1;
    }
}

bool GrasslandScene::isPlayerInGrassArea(int* areaIndex)
{
    // Get player's feet position (collision box)
    QRectF playerFeet(playerPos.x() + 5, playerPos.y() + 30, 25, 18);
    
    // Check if player is in any tall grass area
    for (int i = 0; i < tallGrassItems.size(); ++i) {
        QRectF grassRect = tallGrassItems[i]->rect();
        if (playerFeet.intersects(grassRect)) {
            if (areaIndex) {
                *areaIndex = i;
            }
            qDebug() << "Player entered grass area" << i << "at position" << playerPos;
            return true;
        }
    }
    
    return false;
}

void GrasslandScene::spawnWildPokemon(int grassAreaIndex)
{
    if (grassAreaIndex < 0 || grassAreaIndex >= tallGrassItems.size()) {
        return;
    }
    
    QRectF grassRect = tallGrassItems[grassAreaIndex]->rect();
    
    // Choose a random Pokémon type
    QStringList pokemonTypes = {"Bulbasaur", "Charmander", "Squirtle"};
    QString type = pokemonTypes[QRandomGenerator::global()->bounded(pokemonTypes.size())];
    
    // Get player's position to ensure the Pokémon isn't spawned too close
    QPointF playerCenter(playerPos.x() + 15, playerPos.y() + 20);
    
    // Try to spawn the Pokémon at least 50 pixels away from the player
    const int MIN_DISTANCE = 50;
    int randomX, randomY;
    QPointF pokemonPos;
    int attempts = 0;
    const int MAX_ATTEMPTS = 10;
    qreal dx = 0, dy = 0; // Move declarations outside the loop for later use
    qreal distanceSquared = 0;
    
    do {
        // Choose a random position within the grass area
        randomX = QRandomGenerator::global()->bounded(
            static_cast<int>(grassRect.left() + 30), 
            static_cast<int>(grassRect.right() - 30)
        );
        randomY = QRandomGenerator::global()->bounded(
            static_cast<int>(grassRect.top() + 30), 
            static_cast<int>(grassRect.bottom() - 30)
        );
        
        pokemonPos = QPointF(randomX, randomY);
        
        // Calculate distance from player
        dx = playerCenter.x() - pokemonPos.x();
        dy = playerCenter.y() - pokemonPos.y();
        distanceSquared = dx * dx + dy * dy;
        
        attempts++;
        
        // Accept if distance is good or we've tried too many times
        if (distanceSquared >= MIN_DISTANCE * MIN_DISTANCE || attempts >= MAX_ATTEMPTS) {
            break;
        }
    } while (true);
    
    // Create the wild Pokémon data
    WildPokemon pokemon;
    pokemon.type = type;
    pokemon.position = pokemonPos;
    pokemon.encountered = false;
    
    // Choose sprite based on type - using the correct battle image paths
    QString spriteFile;
    if (type == "Bulbasaur") {
        spriteFile = ":/Dataset/Image/battle/bulbasaur.png";
    } else if (type == "Charmander") {
        spriteFile = ":/Dataset/Image/battle/charmander.png";
    } else { // Squirtle
        spriteFile = ":/Dataset/Image/battle/squirtle.png";
    }
    
    // Load and create sprite item
    QPixmap pokemonPixmap(spriteFile);
    if (!pokemonPixmap.isNull()) {
        // Exactly 40x40 pixels as requested
        pokemonPixmap = pokemonPixmap.scaled(40, 40, Qt::KeepAspectRatio, Qt::SmoothTransformation);
        QGraphicsPixmapItem* spriteItem = scene->addPixmap(pokemonPixmap);
        spriteItem->setPos(pokemon.position.x() - 20, pokemon.position.y() - 20); // Center sprite
        spriteItem->setZValue(10); // Increased zValue to ensure visibility
        spriteItem->setVisible(true); // Explicitly make always visible
        pokemon.spriteItem = spriteItem;
        
        qDebug() << "SUCCESS: Spawned wild" << type << "in grass area" << grassAreaIndex 
             << "at position" << randomX << "," << randomY << "with sprite from" << spriteFile
             << "(distance from player:" << sqrt(dx*dx + dy*dy) << ")";
    } else {
        qDebug() << "ERROR: Failed to load Pokémon sprite from" << spriteFile;
        pokemon.spriteItem = nullptr;
    }
    
    // Add to wild Pokémon list
    wildPokemons.append(pokemon);
}

void GrasslandScene::checkWildPokemonCollision()
{
    // Get player's collision box
    QRectF playerBox(playerPos.x() + 5, playerPos.y() + 10, 25, 35);
    
    for (WildPokemon& pokemon : wildPokemons) {
        // Skip already encountered Pokémon
        if (pokemon.encountered || !pokemon.spriteItem) {
            continue;
        }
        
        // Create Pokémon collision box
        QRectF pokemonBox(
            pokemon.position.x() - 20,
            pokemon.position.y() - 20,
            40, 40
        );
        
        // Check for collision
        if (playerBox.intersects(pokemonBox)) {
            // Mark as encountered to prevent multiple encounters
            pokemon.encountered = true;
            
            // Hide the sprite
            pokemon.spriteItem->setVisible(false);
            
            // Start battle with this Pokémon
            startBattle(pokemon.type);
            break;
        }
    }
}

void GrasslandScene::startBattle(const QString& pokemonType)
{
    qDebug() << "Starting battle with wild" << pokemonType;
    
    // Reset wild Pokemon HP to full at start of battle
    wildPokemonHp = 30;
    
    // First we need to disable movement
    currentPressedKey = 0;
    pressedKeys.clear();
    
    // Stop movement timer
    if (movementTimer && movementTimer->isActive()) {
        movementTimer->stop();
    }
    
    // Store the Pokémon type
    currentBattlePokemonType = pokemonType;

    // Get player's Pokémon
    const QVector<Pokemon*>& playerPokemon = game->getPokemon();
    if (playerPokemon.isEmpty()) {
        // If player has no Pokémon, show warning and return to game
        showDialogue("You have no Pokémon to battle with! Run away!");
        return;
    }

    // Build the selection dialogue text
    QString dialogText = "A wild " + pokemonType + " appeared!\n\nChoose your Pokémon:\n";
    for (int i = 0; i < playerPokemon.size(); i++) {
        dialogText += QString("Press %1: %2\n").arg(i + 1).arg(playerPokemon[i]->getName());
    }
    dialogText += "\nPress ESC to run away";

    // Show the selection dialogue
    showPokemonSelectionDialogue(dialogText);
}

void GrasslandScene::showPokemonSelectionDialogue(const QString& text)
{
    // Remove any existing dialogue box and text
    if (dialogBoxItem) {
        scene->removeItem(dialogBoxItem);
        delete dialogBoxItem;
        dialogBoxItem = nullptr;
    }
    
    if (dialogTextItem) {
        scene->removeItem(dialogTextItem);
        delete dialogTextItem;
        dialogTextItem = nullptr;
    }
    
    // Create the dialogue box using the image
    QPixmap dialogBox(":/Dataset/Image/dialog.png");
    if (dialogBox.isNull()) {
        qDebug() << "Dialog box image not found, creating a fallback rectangle";
        dialogBoxItem = scene->addRect(0, 0, VIEW_WIDTH - 20, 150, QPen(Qt::black), QBrush(QColor(255, 255, 255, 200)));
    } else {
        // Scale dialog box to fit more text
        dialogBox = dialogBox.scaled(VIEW_WIDTH - 20, 150, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
        dialogBoxItem = scene->addPixmap(dialogBox);
    }
    
    // Position the dialogue box at the bottom of the screen
    dialogBoxItem->setPos(cameraPos.x() + 10, cameraPos.y() + VIEW_HEIGHT - dialogBox.height() - 10);
    dialogBoxItem->setZValue(90);
    
    // Create text with appropriate font
    QFont dialogFont("Arial", 11);
    dialogTextItem = scene->addText(text, dialogFont);
    dialogTextItem->setDefaultTextColor(Qt::black);
    
    // Position the text inside the dialogue box with some padding
    float textX = cameraPos.x() + 25;
    float textY = cameraPos.y() + VIEW_HEIGHT - dialogBox.height() + 5;
    dialogTextItem->setPos(textX, textY);
    dialogTextItem->setZValue(91);
    
    // Set text width to wrap long lines
    QTextDocument* doc = dialogTextItem->document();
    doc->setTextWidth(VIEW_WIDTH - 50);
    
    // Set dialogue as active and mark it as a Pokémon selection dialogue
    isDialogueActive = true;
    isPokemonSelectionDialogue = true;
}

void GrasslandScene::showBattleScene()
{
    // Set battle state flag
    inBattleScene = true;
    
    // Clear any existing battle menu items
    for (auto rect : battleMenuRects) {
        if (rect) {
            scene->removeItem(rect);
            delete rect;
        }
    }
    battleMenuRects.clear();
    
    for (auto text : battleMenuTexts) {
        if (text) {
            scene->removeItem(text);
            delete text;
        }
    }
    battleMenuTexts.clear();

    // Load battle scene background
    QPixmap battleBackground(":/Dataset/Image/battle/battle_scene.png");
    if (battleBackground.isNull()) {
        qDebug() << "Failed to load battle scene image!";
        battleBackground = QPixmap(525, 450);
        battleBackground.fill(QColor(100, 100, 200));
    }
    
    // Make sure the scene has the battle background
    if (!battleSceneItem) {
        battleSceneItem = scene->addPixmap(battleBackground);
        battleSceneItem->setZValue(200);
    } else {
        battleSceneItem->setPixmap(battleBackground);
        battleSceneItem->setVisible(true);
    }
    
    // Position battle scene relative to camera view
    battleSceneItem->setPos(cameraPos);
    
    // Add player's Pokémon back view on the left and show its stats
    if (!game->getPokemon().isEmpty()) {
        Pokemon* playerPokemon = game->getPokemon().first();
        QString playerPokemonName = playerPokemon->getName();
        QString backImagePath = QString(":/Dataset/Image/battle/%1_back.png").arg(playerPokemonName.toLower());
        QPixmap playerPokemonImage(backImagePath);
        
        if (!playerPokemonImage.isNull()) {
            playerPokemonImage = playerPokemonImage.scaled(120, 120, Qt::KeepAspectRatio, Qt::SmoothTransformation);
            QGraphicsPixmapItem* playerPokemonItem = scene->addPixmap(playerPokemonImage);
            playerPokemonItem->setPos(cameraPos.x() + 50, cameraPos.y() + 200); // Position on left side
            playerPokemonItem->setZValue(201);
            bagPokemonSprites.append(playerPokemonItem);

            // Add player Pokémon stats (name, level, HP)
            float hpBarY = cameraPos.y() + 180;
            
            // Add HP bar for player's Pokemon
            int currentHp = playerPokemon->getCurrentHp();
            int maxHp = playerPokemon->getMaxHp();
            float hpPercentage = static_cast<float>(currentHp) / maxHp;
            
            QGraphicsRectItem* hpBarBg = scene->addRect(0, 0, 100, 10, QPen(Qt::black), QBrush(Qt::lightGray));
            QGraphicsRectItem* hpBarFg = scene->addRect(0, 0, 100 * hpPercentage, 10, QPen(Qt::transparent), 
                QBrush(hpPercentage > 0.5 ? Qt::green : (hpPercentage > 0.2 ? Qt::yellow : Qt::red)));
            
            hpBarBg->setPos(cameraPos.x() + 50, hpBarY);
            hpBarFg->setPos(cameraPos.x() + 50, hpBarY);
            hpBarBg->setZValue(202);
            hpBarFg->setZValue(203);
            battleMenuRects.append(hpBarBg);
            battleMenuRects.append(hpBarFg);

            // Add stats text
            QString statsText = QString("%1  Lv%2\nHP: %3/%4")
                .arg(playerPokemonName)
                .arg(playerPokemon->getLevel())
                .arg(currentHp)
                .arg(maxHp);
            
            QGraphicsTextItem* statsTextItem = scene->addText(statsText, QFont("Arial", 12, QFont::Bold));
            statsTextItem->setDefaultTextColor(Qt::black);
            statsTextItem->setPos(cameraPos.x() + 50, hpBarY - 40);
            statsTextItem->setZValue(202);
            battleMenuTexts.append(statsTextItem);
        }
    }
    
    // Add wild Pokémon image on the right and show its stats
    QString pokemonImagePath;
    if (currentBattlePokemonType == "Bulbasaur") {
        pokemonImagePath = ":/Dataset/Image/battle/bulbasaur.png";
    } else if (currentBattlePokemonType == "Charmander") {
        pokemonImagePath = ":/Dataset/Image/battle/charmander.png";
    } else { // Squirtle
        pokemonImagePath = ":/Dataset/Image/battle/squirtle.png";
    }
    
    QPixmap pokemonImage(pokemonImagePath);
    if (!pokemonImage.isNull()) {
        pokemonImage = pokemonImage.scaled(120, 120, Qt::KeepAspectRatio, Qt::SmoothTransformation);
        QGraphicsPixmapItem* pokemonItem = scene->addPixmap(pokemonImage);
        pokemonItem->setPos(cameraPos.x() + 350, cameraPos.y() + 150);
        pokemonItem->setZValue(201);
        bagPokemonSprites.append(pokemonItem);

        // Add wild Pokémon stats
        float wildHpBarY = cameraPos.y() + 130;
        
        // Add HP bar for wild Pokemon
        float wildHpPercentage = static_cast<float>(wildPokemonHp) / 30;
        QGraphicsRectItem* wildHpBarBg = scene->addRect(0, 0, 100, 10, QPen(Qt::black), QBrush(Qt::lightGray));
        QGraphicsRectItem* wildHpBarFg = scene->addRect(0, 0, 100 * wildHpPercentage, 10, QPen(Qt::transparent), 
            QBrush(wildHpPercentage > 0.5 ? Qt::green : (wildHpPercentage > 0.2 ? Qt::yellow : Qt::red)));
        
        wildHpBarBg->setPos(cameraPos.x() + 350, wildHpBarY);
        wildHpBarFg->setPos(cameraPos.x() + 350, wildHpBarY);
        wildHpBarBg->setZValue(202);
        wildHpBarFg->setZValue(203);
        battleMenuRects.append(wildHpBarBg);
        battleMenuRects.append(wildHpBarFg);

        // Add wild Pokemon stats text
        QString wildStatsText = QString("Wild %1  Lv1\nHP: %2/30")
            .arg(currentBattlePokemonType)
            .arg(wildPokemonHp);
        
        QGraphicsTextItem* wildStatsTextItem = scene->addText(wildStatsText, QFont("Arial", 12, QFont::Bold));
        wildStatsTextItem->setDefaultTextColor(Qt::black);
        wildStatsTextItem->setPos(cameraPos.x() + 350, wildHpBarY - 40);
        wildStatsTextItem->setZValue(202);
        battleMenuTexts.append(wildStatsTextItem);
    }

    // Create battle menu at the bottom
    QString pokemonName = game->getPokemon().isEmpty() ? "POKEMON" : game->getPokemon().first()->getName().toUpper();
    QString promptText = QString("What will\n%1 do?").arg(pokemonName);
    
    QGraphicsTextItem* promptTextItem = scene->addText(promptText, QFont("Arial", 12, QFont::Bold));
    promptTextItem->setDefaultTextColor(Qt::black);
    promptTextItem->setPos(cameraPos.x() + 25, cameraPos.y() + VIEW_HEIGHT - 120);
    promptTextItem->setZValue(202);
    battleMenuTexts.append(promptTextItem);
    
    // Create the 4 menu options
    QStringList menuOptions = {"FIGHT", "BAG", "POKéMON", "RUN"};
    float startX = cameraPos.x() + VIEW_WIDTH / 2;
    float startY = cameraPos.y() + VIEW_HEIGHT - 120;
    float optionWidth = (VIEW_WIDTH / 2) / 2;
    float optionHeight = 50;
    
    for (int i = 0; i < 4; i++) {
        int row = i / 2;
        int col = i % 2;
        
        float x = startX + col * optionWidth;
        float y = startY + row * optionHeight;
        
        QGraphicsTextItem* optionText = scene->addText(menuOptions[i], QFont("Arial", 12, QFont::Bold));
        optionText->setDefaultTextColor(Qt::black);
        optionText->setPos(x + 20, y + 10);
        optionText->setZValue(203);
        battleMenuTexts.append(optionText);
        
        // Add selection marker for current option
        if (static_cast<BattleOption>(i) == selectedBattleOption) {
            QPolygonF triangle;
            triangle << QPointF(0, 0) << QPointF(10, 5) << QPointF(0, 10);
            
            QGraphicsPolygonItem* marker = scene->addPolygon(triangle, QPen(Qt::black), QBrush(Qt::black));
            marker->setPos(x + 5, y + 15);
            marker->setZValue(204);
            battleMenuRects.append(marker);
        }
    }
}

void GrasslandScene::exitBattleScene()
{
    qDebug() << "Exiting battle scene";
    
    // Clean up battle scene items
    if (battleSceneItem) {
        battleSceneItem->setVisible(false);
    }
    
    // Clean up menu items
    for (auto rect : battleMenuRects) {
        if (rect) {
            scene->removeItem(rect);
            delete rect;
        }
    }
    battleMenuRects.clear();
    
    for (auto text : battleMenuTexts) {
        if (text) {
            scene->removeItem(text);
            delete text;
        }
    }
    battleMenuTexts.clear();
    
    // Reset battle state
    inBattleScene = false;
    
    // Clean up wild Pokémon in battle
    for (auto sprite : bagPokemonSprites) {
        if (sprite) {
            scene->removeItem(sprite);
            delete sprite;
        }
    }
    bagPokemonSprites.clear();
    
    for (auto text : bagPokemonNames) {
        if (text) {
            scene->removeItem(text);
            delete text;
        }
    }
    bagPokemonNames.clear();
    
    // Resume player movement
    if (movementTimer && !movementTimer->isActive()) {
        movementTimer->start(60);
    }
    
    qDebug() << "Battle scene exited";
}

void GrasslandScene::showBattleBag()
{
    // Set battle bag state
    isBattleBagOpen = true;
    
    // Clear any existing battle menu items
    for (auto rect : battleMenuRects) {
        if (rect) {
            scene->removeItem(rect);
            delete rect;
        }
    }
    battleMenuRects.clear();
    
    for (auto text : battleMenuTexts) {
        if (text) {
            scene->removeItem(text);
            delete text;
        }
    }
    battleMenuTexts.clear();

    // Get player's inventory
    QMap<QString, int> inventory = game->getItems();

    // Create text showing available items with counts
    QString bagText = "Choose an item to use:\n\n";
    
    // Add Poké Ball option
    int pokeballs = inventory.value("Poké Ball", 0);
    if (pokeballs > 0) {
        bagText += QString("Press 1: Use Poké Ball (%1 left)\n").arg(pokeballs);
    }
    
    // Add Potion option
    int potions = inventory.value("Potion", 0);
    if (potions > 0) {
        bagText += QString("Press 2: Use Potion (%1 left)\n").arg(potions);
    }
    
    // Add Ether option
    int ethers = inventory.value("Ether", 0);
    if (ethers > 0) {
        bagText += QString("Press 3: Use Ether (%1 left)\n").arg(ethers);
    }
    
    bagText += "\nPress B to return";

    // Create and position the text
    QFont textFont("Arial", 12);
    QGraphicsTextItem* bagTextItem = scene->addText(bagText, textFont);
    bagTextItem->setDefaultTextColor(Qt::black);
    bagTextItem->setPos(cameraPos.x() + 25, cameraPos.y() + VIEW_HEIGHT - 120);
    bagTextItem->setZValue(202);
    battleMenuTexts.append(bagTextItem);
}

void GrasslandScene::handleBagSelection(int itemIndex)
{
    // Get items as a copy since we can't modify the original directly
    QMap<QString, int> inventory = game->getItems();
    QString itemName;
    bool itemUsed = false;
    QString resultMessage;
    
    // Get player's active Pokémon
    const QVector<Pokemon*>& playerPokemon = game->getPokemon();
    if (playerPokemon.isEmpty()) {
        return;
    }
    Pokemon* activePokemon = playerPokemon.first();
    
    switch (itemIndex) {
        case 1: // Poké Ball
            if (inventory.value("Poké Ball", 0) > 0) {
                itemName = "Poké Ball";
                // TODO: Implement catching mechanics
                itemUsed = true;
                resultMessage = "Used Poké Ball!";
            }
            break;
            
        case 2: // Potion
            if (inventory.value("Potion", 0) > 0) {
                itemName = "Potion";
                // Heal 10 HP
                int currentHp = activePokemon->getCurrentHp();
                int maxHp = activePokemon->getMaxHp();
                if (currentHp < maxHp) { // Only heal if not at max HP
                    int healAmount = 10;
                    int newHp = qMin(currentHp + healAmount, maxHp);
                    activePokemon->setCurrentHp(newHp);
                    itemUsed = true;
                    resultMessage = QString("%1 recovered 10 HP!").arg(activePokemon->getName());
                    
                    // Update inventory immediately
                    inventory["Potion"]--;
                    if (inventory["Potion"] <= 0) {
                        inventory.remove("Potion");
                    }
                    game->setItems(inventory);
                    
                    // Show recovery message
                    QGraphicsTextItem* messageText = scene->addText(resultMessage, QFont("Arial", 12, QFont::Bold));
                    messageText->setDefaultTextColor(Qt::black);
                    messageText->setPos(cameraPos.x() + 50, cameraPos.y() + 150); // Above player's Pokémon
                    messageText->setZValue(205);
                    battleMenuTexts.append(messageText);
                    
                    // Update battle scene to show new HP after 2 seconds
                    QTimer::singleShot(2000, [this]() {
                        showBattleScene();
                        // Start wild Pokémon's turn after showing updated HP
                        QTimer::singleShot(2000, [this]() {
                            isBattleBagOpen = false;
                            wildPokemonTurn();
                        });
                    });
                    return;
                } else {
                    resultMessage = "HP is already full!";
                    itemUsed = false;
                }
            }
            break;
            
        case 3: // Ether
            if (inventory.value("Ether", 0) > 0) {
                itemName = "Ether";
                // Restore PP of all moves
                QVector<Pokemon::Move>& moves = const_cast<QVector<Pokemon::Move>&>(activePokemon->getMoves());
                for (Pokemon::Move& move : moves) {
                    move.pp = 20; // Reset to max PP
                }
                itemUsed = true;
                resultMessage = "All move PP is restored now!";
                
                // Update inventory immediately
                inventory["Ether"]--;
                if (inventory["Ether"] <= 0) {
                    inventory.remove("Ether");
                }
                game->setItems(inventory);
                
                // Show PP restore message
                QGraphicsTextItem* messageText = scene->addText(resultMessage, QFont("Arial", 12, QFont::Bold));
                messageText->setDefaultTextColor(Qt::black);
                messageText->setPos(cameraPos.x() + 50, cameraPos.y() + 150); // Above player's Pokémon
                messageText->setZValue(205);
                battleMenuTexts.append(messageText);
                
                // Wait 3 seconds before wild Pokémon's turn
                QTimer::singleShot(3000, [this]() {
                    isBattleBagOpen = false;
                    wildPokemonTurn();
                });
                return;
            }
            break;
    }
    
    if (!itemUsed && !resultMessage.isEmpty()) {
        // Show error message (e.g., HP already full)
        QGraphicsTextItem* messageText = scene->addText(resultMessage, QFont("Arial", 12, QFont::Bold));
        messageText->setDefaultTextColor(Qt::black);
        messageText->setPos(cameraPos.x() + 50, cameraPos.y() + 150); // Above player's Pokémon
        messageText->setZValue(205);
        battleMenuTexts.append(messageText);
        
        // Return to battle menu after a short delay
        QTimer::singleShot(1000, [this]() {
            isBattleBagOpen = false;
            showBattleScene();
        });
    } else if (!itemUsed) {
        // If no message, just return to battle menu
        isBattleBagOpen = false;
        showBattleScene();
    }
}

void GrasslandScene::showMoveSelection()
{
    // Set move selection state
    isMoveSelectionActive = true;
    
    // Clear any existing battle menu items
    for (auto rect : battleMenuRects) {
        if (rect) {
            scene->removeItem(rect);
            delete rect;
        }
    }
    battleMenuRects.clear();
    
    for (auto text : battleMenuTexts) {
        if (text) {
            scene->removeItem(text);
            delete text;
        }
    }
    battleMenuTexts.clear();

    // Get player's active Pokémon
    const QVector<Pokemon*>& playerPokemon = game->getPokemon();
    if (playerPokemon.isEmpty()) {
        return;
    }

    Pokemon* activePokemon = playerPokemon.first();
    const QVector<Pokemon::Move>& moves = activePokemon->getMoves();

    // Create text showing available moves based on level
    QString moveText;
    int pokemonLevel = activePokemon->getLevel();
    
    // At level 1, only show the first move
    if (pokemonLevel == 1) {
        if (!moves.isEmpty()) {
            // Show move with PP, gray out if PP is 0
            QString moveStr = QString("Press 1: %1 (PP: %2/20)")
                .arg(moves[0].name)
                .arg(moves[0].pp);
            
            if (moves[0].pp <= 0) {
                moveStr = QString("[OUT OF PP] %1").arg(moveStr);
            }
            moveText += moveStr + "\n";
        }
    } else {
        // At level 2, show both moves
        for (int i = 0; i < moves.size(); ++i) {
            // Show move with PP, gray out if PP is 0
            QString moveStr = QString("Press %1: %2 (PP: %3/20)")
                .arg(i + 1)
                .arg(moves[i].name)
                .arg(moves[i].pp);
            
            if (moves[i].pp <= 0) {
                moveStr = QString("[OUT OF PP] %1").arg(moveStr);
            }
            moveText += moveStr + "\n";
        }
    }
    
    moveText += "\nPress C: Do Nothing\n";
    moveText += "Press B to return";

    // Create and position the text
    QFont textFont("Arial", 12);
    QGraphicsTextItem* moveTextItem = scene->addText(moveText, textFont);
    moveTextItem->setDefaultTextColor(Qt::black);
    moveTextItem->setPos(cameraPos.x() + 25, cameraPos.y() + VIEW_HEIGHT - 120);
    moveTextItem->setZValue(202);
    battleMenuTexts.append(moveTextItem);
}

void GrasslandScene::handleMoveSelection(int moveIndex)
{
    // Get player's active Pokémon
    const QVector<Pokemon*>& playerPokemon = game->getPokemon();
    if (playerPokemon.isEmpty()) {
        return;
    }

    Pokemon* activePokemon = playerPokemon.first();
    const QVector<Pokemon::Move>& moves = activePokemon->getMoves();

    // Clear move selection text
    isMoveSelectionActive = false;
    showBattleScene();

    // Handle "Do Nothing" option
    if (moveIndex == -1) {
        // Start timer for opponent's turn without showing any text
        if (!battleTimer) {
            battleTimer = new QTimer(this);
            connect(battleTimer, &QTimer::timeout, this, &GrasslandScene::wildPokemonTurn);
            battleTimer->setSingleShot(true);
        }
        battleTimer->start(1000);
        return;
    }

    // Check if move index is valid and has PP
    if (moveIndex >= moves.size() || moves[moveIndex].pp <= 0) {
        return;
    }

    // Get the selected move
    const Pokemon::Move& selectedMove = moves[moveIndex];

    // Calculate damage using the formula: Damage = (Power + User's Attack - Opponent's Defense) × Level
    int power = selectedMove.power; // Use the move's actual power
    int userAttack = activePokemon->getAttack();
    int opponentDefense = 5; // Base defense for wild Pokémon
    int damage = (power + userAttack - opponentDefense) * activePokemon->getLevel();
    if (damage < 1) damage = 1; // Minimum damage is 1

    // Decrease PP (need to cast away const to modify)
    const_cast<Pokemon::Move&>(selectedMove).pp--;

    // Apply damage to wild Pokemon
    wildPokemonHp -= damage;
    if (wildPokemonHp < 0) wildPokemonHp = 0;

    // Show the move and damage in battle scene
    QString moveText = QString("%1 used %2!\nDealt %3 damage!")
        .arg(activePokemon->getName())
        .arg(selectedMove.name)
        .arg(damage);
    
    QGraphicsTextItem* actionText = scene->addText(moveText, QFont("Arial", 12, QFont::Bold));
    actionText->setDefaultTextColor(Qt::black);
    actionText->setPos(cameraPos.x() + 25, cameraPos.y() + VIEW_HEIGHT - 90);
    actionText->setZValue(205);
    battleMenuTexts.append(actionText);

    // Update battle display to show new HP
    showBattleScene();

    // Check if battle should end
    if (wildPokemonHp <= 0) {
        // Level up the player's Pokémon
        activePokemon->setLevel(activePokemon->getLevel() + 1);
        
        // Show victory and level up message in battle scene
        QString victoryText = QString("%1 won the battle!\n%1 grew to level %2!")
            .arg(activePokemon->getName())
            .arg(activePokemon->getLevel());
            
        QGraphicsTextItem* battleText = scene->addText(
            victoryText,
            QFont("Arial", 12, QFont::Bold)
        );
        battleText->setDefaultTextColor(Qt::black);
        battleText->setPos(cameraPos.x() + 25, cameraPos.y() + VIEW_HEIGHT - 90);
        battleText->setZValue(205);
        battleMenuTexts.append(battleText);
        
        // Exit battle scene after a delay
        QTimer::singleShot(2000, [this]() {
            exitBattleScene();
        });
        return;
    }

    // Start timer for opponent's turn
    if (!battleTimer) {
        battleTimer = new QTimer(this);
        connect(battleTimer, &QTimer::timeout, this, &GrasslandScene::wildPokemonTurn);
        battleTimer->setSingleShot(true);
    }
    battleTimer->start(2000);
}

void GrasslandScene::wildPokemonTurn()
{
    // Get player's active Pokémon
    const QVector<Pokemon*>& playerPokemon = game->getPokemon();
    if (playerPokemon.isEmpty()) {
        return;
    }

    Pokemon* activePokemon = playerPokemon.first();

    // Calculate damage using the same formula: Damage = (Power + User's Attack - Opponent's Defense) × Level
    int power = 10; // Base power for wild Pokémon moves
    int wildAttack = 5; // Same attack as player Pokémon
    int playerDefense = 5; // Base defense for player's Pokémon
    int wildLevel = 1; // Wild Pokémon are always level 1
    int damage = (power + wildAttack - playerDefense) * wildLevel;
    if (damage < 1) damage = 1; // Minimum damage is 1

    QString move = "Tackle"; // Default move for wild Pokémon
    
    // Split the move text into two separate text items for better visibility
    QString moveText = QString("Wild %1 used %2!").arg(currentBattlePokemonType).arg(move);
    QString damageText = QString("Dealt %1 damage!").arg(damage);
    
    // Add move text - position at the middle of the view
    QGraphicsTextItem* moveTextItem = scene->addText(moveText, QFont("Arial", 12, QFont::Bold));
    moveTextItem->setDefaultTextColor(Qt::black);
    moveTextItem->setPos(cameraPos.x() + 282, cameraPos.y() + 20); // Center position
    moveTextItem->setZValue(205);
    battleMenuTexts.append(moveTextItem);

    // Add damage text below move text
    QGraphicsTextItem* damageTextItem = scene->addText(damageText, QFont("Arial", 12, QFont::Bold));
    damageTextItem->setDefaultTextColor(Qt::black);
    damageTextItem->setPos(cameraPos.x() + 282, cameraPos.y() + 40); // Below move text, same horizontal alignment
    damageTextItem->setZValue(205);
    battleMenuTexts.append(damageTextItem);

    // Apply damage and ensure HP doesn't go below 0
    int currentHp = activePokemon->getCurrentHp();
    int newHp = currentHp - damage;
    if (newHp < 0) newHp = 0;
    activePokemon->setCurrentHp(newHp);

    // Update battle display after a short delay
    QTimer::singleShot(2000, [this]() {
        showBattleScene();

        // Check if battle should end
        const QVector<Pokemon*>& playerPokemon = game->getPokemon();
        if (!playerPokemon.isEmpty() && playerPokemon.first()->getCurrentHp() <= 0) {
            // Show defeat message in battle scene - moved left
            QGraphicsTextItem* defeatText = scene->addText(
                QString("Your %1 fainted!").arg(playerPokemon.first()->getName()),
                QFont("Arial", 12, QFont::Bold)
            );
            defeatText->setDefaultTextColor(Qt::black);
            defeatText->setPos(cameraPos.x() + 290, cameraPos.y() + 20); // Moved left to match wild Pokemon text
            defeatText->setZValue(205);
            battleMenuTexts.append(defeatText);
            
            // Exit battle scene after a delay without showing additional text
            QTimer::singleShot(2000, [this]() {
                exitBattleScene();
            });
            return;
        }

        // Return to battle menu
        selectedBattleOption = FIGHT;
        showBattleScene();
    });
}

// ... existing code ... 
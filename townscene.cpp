#include "townscene.h"
#include "game.h"
#include <QDebug>
#include <QGraphicsTextItem>
#include <QFont>
#include <QTextDocument>
#include <cmath>

// Define constants for the scene size - must match those from Scene class
const int SCENE_WIDTH = 1000;
const int SCENE_HEIGHT = 1000;
const int VIEW_WIDTH = 525;   // Reset to original view width (smaller than town)
const int VIEW_HEIGHT = 450;  // Reset to original view height (smaller than town)

TownScene::TownScene(Game *game, QGraphicsScene *scene, QObject *parent)
    : Scene(game, scene, parent)
{
    // Create update timer
    updateTimer = new QTimer(this);
    connect(updateTimer, &QTimer::timeout, this, &TownScene::updateScene);
    updateTimer->start(100);  // Update every 100ms
    
    // Create movement timer for continuous walking
    movementTimer = new QTimer(this);
    connect(movementTimer, &QTimer::timeout, this, &TownScene::processMovement);
    movementTimer->setInterval(60);  // smaller number = faster walking speed
}

TownScene::~TownScene()
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

void TownScene::initialize()
{
    qDebug() << "Initializing Town Scene";

    // Set player position to the exact center of the 1000x1000 town
    playerPos = QPointF(TOWN_WIDTH / 2, TOWN_HEIGHT / 2);
    qDebug() << "Player position set to center of town:" << playerPos.x() << "," << playerPos.y();

    // Reset movement state
    currentPressedKey = 0;
    pressedKeys.clear();
    if (movementTimer && movementTimer->isActive()) {
        movementTimer->stop();
    }

    // Create scene elements
    createBackground();
    createBarriers();
    createPlayer();

    // Set initial camera position to center on player
    updateCamera();

    // Start update timer
    updateTimer->start(16); // 60 FPS
    // Start movement timer for continuous movement
    movementTimer->start(100);
}

void TownScene::cleanup()
{
    // Stop update timer
    updateTimer->stop();

    // Clear bag display items explicitly
    clearBagDisplayItems();

    // Clear scene
    scene->clear();

    // Reset pointers
    backgroundItem = nullptr;
    playerItem = nullptr;
    barrierItems.clear();
}

void TownScene::createBackground()
{
    // First create a large black background for the entire scene
    QGraphicsRectItem* blackBackground = scene->addRect(0, 0, SCENE_WIDTH, SCENE_HEIGHT, 
        QPen(Qt::transparent), QBrush(Qt::black));
    blackBackground->setZValue(-1);
    qDebug() << "Black background created with size:" << SCENE_WIDTH << "x" << SCENE_HEIGHT;

    // Load town background
    QPixmap background(":/Dataset/Image/scene/Town.png");

    if (background.isNull()) {
        qDebug() << "Town background image not found. Check the path.";
        background = QPixmap(TOWN_WIDTH, TOWN_HEIGHT);
        background.fill(Qt::white);
    } else {
        qDebug() << "Town background loaded successfully, size:" << background.width() << "x" << background.height();
        
        // Scale the image to fill 1000x1000 town area
        if (background.width() != TOWN_WIDTH || background.height() != TOWN_HEIGHT) {
            background = background.scaled(TOWN_WIDTH, TOWN_HEIGHT, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
            qDebug() << "Town background scaled to:" << background.width() << "x" << background.height();
        }
    }

    // Position town background at (0,0) in scene
    backgroundItem = scene->addPixmap(background);
    backgroundItem->setPos(0, 0);
    backgroundItem->setZValue(0);

    qDebug() << "Town background positioned for scrolling view";
    
    // Make sure the scene background is also black
    scene->setBackgroundBrush(Qt::black);
}

void TownScene::createPlayer()
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

void TownScene::createBarriers()
{
    // Define barriers for the town based on 1000x1000 dimensions and the reference image
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
        
        
       
        QRect(209, 698, 42, 46),  // Bottom-left bulletin board
        QRect(377, 548, 45, 45),  // Center-left fence bulletin board
        QRect(669, 801, 45, 45),  // Bottom fence bulletin board
        
        
        
        QRect(550, 470, 281, 225),  // Center main building
        
   
        
        QRect(297, 849, 152, 145),  // Lake at bottom
        

    };

    // Add barriers (with visible red outlines for debugging)
    for (const QRect &rect : barrierRects) {
        QGraphicsRectItem *barrier = scene->addRect(rect, QPen(Qt::red, 1), QBrush(Qt::transparent));
        barrier->setZValue(5); // Higher zValue to be visible for debugging
        barrierItems.append(barrier);
    }
    
    // Create bulletin boards with green color - MATCH EXACTLY with barrier positions
    // First bulletin board (bottom-left)
    QRect bulletinBoard1(209, 698, 42, 46);
    QGraphicsRectItem *board1 = scene->addRect(bulletinBoard1, QPen(Qt::darkGreen, 2), QBrush(QColor(0, 128, 0, 100)));
    board1->setZValue(2); // Below player but visible
    bulletinBoardItems.append(board1);
    
    // Second bulletin board (center-left fence)
    QRect bulletinBoard2(377, 548, 45, 45);
    QGraphicsRectItem *board2 = scene->addRect(bulletinBoard2, QPen(Qt::darkGreen, 2), QBrush(QColor(0, 128, 0, 100)));
    board2->setZValue(2);
    bulletinBoardItems.append(board2);
    
    // Third bulletin board (bottom fence)
    QRect bulletinBoard3(669, 801, 45, 45);
    QGraphicsRectItem *board3 = scene->addRect(bulletinBoard3, QPen(Qt::darkGreen, 2), QBrush(QColor(0, 128, 0, 100)));
    board3->setZValue(2);
    bulletinBoardItems.append(board3);
    
    // Create lab transition portal (blue box)
    QRect labPortalRect(669, 700, 45, 45);
    QGraphicsRectItem *labPortal = scene->addRect(labPortalRect, QPen(Qt::blue, 2), QBrush(QColor(0, 0, 255, 100)));
    labPortal->setZValue(2); // Below player but visible
    labPortalItem = labPortal;
    
    // Create grassland transition portal (blue box)
    QRect grasslandPortalRect(490, 0, 90, 90);
    QGraphicsRectItem *grasslandPortal = scene->addRect(grasslandPortalRect, QPen(Qt::blue, 2), QBrush(QColor(0, 0, 255, 100)));
    grasslandPortal->setZValue(2); // Below player but visible
    grasslandPortalItem = grasslandPortal;
    
    qDebug() << "Created" << barrierItems.size() << "barriers," << bulletinBoardItems.size() 
             << "bulletin boards, and 2 portals for town";
}

void TownScene::handleKeyPress(int key)
{
    qDebug() << "Town scene key pressed:" << key;

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
            } else if (playerPos.x() > TOWN_WIDTH - 25) {
                playerPos.setX(TOWN_WIDTH - 25);
            }

            if (playerPos.y() < 0) {
                playerPos.setY(0);
            } else if (playerPos.y() > TOWN_HEIGHT - 48) {
                playerPos.setY(TOWN_HEIGHT - 48);
            }
            
            // Collision check
            QRectF playerRect(playerPos.x() + 5, playerPos.y() + 30, 25, 18);
            bool collision = false;
            
            for (const QGraphicsRectItem* barrier : barrierItems) {
                QRectF barrierRect = barrier->rect();
                if (playerRect.intersects(barrierRect)) {
                    collision = true;
                    break;
                }
            }
            
            if (collision) {
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
        // Check if player is near a bulletin board
        int boardIndex = -1;
        if (isPlayerNearBulletinBoard(boardIndex)) {
            // Show the Pallet Town message for bulletin boards
            showDialogue("This is Pallet Town. Begin your adventure!");
            qDebug() << "Activated bulletin board dialogue at index:" << boardIndex;
        } else {
            // Only show a generic message if near an NPC or other interactive object
            // For now, don't show any message when pressing A randomly
            qDebug() << "Player pressed A but not near any bulletin board";
        }
    }
}

void TownScene::handleKeyRelease(int key)
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

void TownScene::processMovement()
{
    // Do nothing if no key is pressed or dialogue/bag is open
    if (currentPressedKey == 0 || isDialogueActive || isBagOpen) {
        static int stepCounter = 0;
        stepCounter = 0; // Reset counter when not moving
        return;
    }
    
    QPointF prevPos = playerPos;
    bool moved = false;

    // Calculate the position to center the town in the larger scene
    float townOffsetX = (SCENE_WIDTH - TOWN_WIDTH) / 2;
    float townOffsetY = (SCENE_HEIGHT - TOWN_HEIGHT) / 2;

    // Speed increases after holding the key for some time
    static int stepCounter = 0;
    int moveSpeed = 8; // Increased by 10% from 7
    
    // Increase speed after a few steps
    if (stepCounter > 3) {
        moveSpeed = 11; // Increased by 10% from 10
    }
    stepCounter++;

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
        // Boundary checking - don't allow player to go outside town area
        if (playerPos.x() < 0) {
            playerPos.setX(0);
        } else if (playerPos.x() > TOWN_WIDTH - 25) {
            playerPos.setX(TOWN_WIDTH - 25);
        }

        if (playerPos.y() < 0) {
            playerPos.setY(0);
        } else if (playerPos.y() > TOWN_HEIGHT - 48) {
            playerPos.setY(TOWN_HEIGHT - 48);
        }

        // Check collision with barriers using a smaller hitbox at player's feet
        QRectF playerRect(playerPos.x() + 5, playerPos.y() + 30, 25, 18);
        bool collision = false;
        
        for (const QGraphicsRectItem* barrier : barrierItems) {
            QRectF barrierRect = barrier->rect();
            if (playerRect.intersects(barrierRect)) {
                collision = true;
                break;
            }
        }

        if (collision) {
            playerPos = prevPos;
            stepCounter = 0; // Reset counter when collision occurs
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
            
            // Check if player is now on the lab portal - automatic transport
            if (isPlayerNearLabPortal()) {
                qDebug() << "Player walked into the portal to return to lab";
                
                // Reset movement state before changing scene
                currentPressedKey = 0;
                pressedKeys.clear();
                movementTimer->stop();
                
                game->changeScene(GameState::LABORATORY);
                return;
            }
        }
    } else {
        stepCounter = 0; // Reset counter if no movement happened
    }
}

void TownScene::updateScene()
{
    // If bag is open or dialogue is active, don't update
    if (isBagOpen || isDialogueActive) {
        return;
    }

    // Check if player is on the lab portal, and if so, change scene
    if (isPlayerNearLabPortal()) {
        qDebug() << "Player on lab portal, switching scene...";
        // Switch to lab scene
        game->changeScene(GameState::LABORATORY);
        return;
    }
    
    // Check if player is on the grassland portal, and if so, change scene
    if (isPlayerNearGrasslandPortal()) {
        qDebug() << "Player on grassland portal, switching scene...";
        // Switch to grassland scene
        game->changeScene(GameState::GRASSLAND);
        return;
    }

    // Other update logic...
}

void TownScene::updatePlayerSprite()
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

void TownScene::updateCamera()
{
    // Make sure playerItem exists
    if (!playerItem) return;
    
    // Get player position (center of player)
    QPointF playerCenter = playerPos + QPointF(17.5, 24); // Center of the player sprite
    
    // Calculate desired camera position (centered on player)
    QPointF targetCameraPos;
    targetCameraPos.setX(playerCenter.x() - VIEW_WIDTH / 2);
    targetCameraPos.setY(playerCenter.y() - VIEW_HEIGHT / 2);
    
    // Implement the camera behavior shown in the reference image:
    // 1. The character stays centered until reaching the background boundary
    // 2. When boundary is reached, window boundary won't exceed background boundary
    // 3. If player moves away from boundary, window centers again
    
    // Calculate boundary constraints
    float minCameraX = 0; // Left boundary
    float maxCameraX = TOWN_WIDTH - VIEW_WIDTH; // Right boundary
    float minCameraY = 0; // Top boundary
    float maxCameraY = TOWN_HEIGHT - VIEW_HEIGHT; // Bottom boundary
    
    // Apply constraints - prevent camera from going outside town boundaries
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

bool TownScene::checkCollision()
{
    // Boundary checking
    if (playerPos.x() < 0 || playerPos.x() > TOWN_WIDTH - 35 ||
        playerPos.y() < 0 || playerPos.y() > TOWN_HEIGHT - 48) {
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

void TownScene::updatePlayerPosition()
{
    if (playerItem) {
        playerItem->setPos(playerPos);
        // Camera will follow player
        updateCamera();
    }
}

void TownScene::toggleBag()
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

void TownScene::clearBagDisplayItems()
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

void TownScene::updateBagDisplay()
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

    // Get the player's Pokémon collection
    const QVector<Pokemon*>& playerPokemon = game->getPokemon();
    if (playerPokemon.isEmpty()) {
        qDebug() << "No Pokémon in player's collection to display";
        return; // Return here but after drawing the bag background
    }

    qDebug() << "Player has" << playerPokemon.size() << "Pokémon:";
    for (int i = 0; i < playerPokemon.size(); i++) {
        qDebug() << i << ":" << playerPokemon[i]->getName() << "with image path:" << playerPokemon[i]->getImagePath();
    }

    // Display each Pokémon in the bag
    const int ROW_HEIGHT = 40;
    const int ROW_SPACING = 15;
    
    // Start at a higher position for first row
    float startY = bagY + 5;
    
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
        nameText->setZValue(101);
        
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
        pokemonSprite->setZValue(101);
        bagPokemonSprites.append(pokemonSprite);
        
        qDebug() << "Added" << pokemon->getName() << "to bag at row" << i 
                 << "text at:" << textX << "," << textY
                 << "sprite at:" << spriteX << "," << spriteY;
    }

    qDebug() << "Bag display updated with" << bagPokemonSprites.size() << "Pokémon";
}

void TownScene::showDialogueBox(const QString &text)
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
        dialogBoxItem = scene->addRect(0, 0, VIEW_WIDTH, 100, QPen(Qt::black), QBrush(QColor(255, 255, 255, 200)));
    } else {
        dialogBoxItem = scene->addPixmap(dialogBox);
    }
    
    // Position the dialogue box at the bottom of the screen
    dialogBoxItem->setPos(cameraPos.x(), cameraPos.y() + VIEW_HEIGHT - dialogBox.height());
    dialogBoxItem->setZValue(90); // Above most elements
    
    // Create text with appropriate font
    QFont dialogFont("Arial", 12);
    dialogTextItem = scene->addText(text, dialogFont);
    dialogTextItem->setDefaultTextColor(Qt::black);
    
    // Position the text inside the dialogue box with some padding
    float textX = cameraPos.x() + 20;
    float textY = cameraPos.y() + VIEW_HEIGHT - dialogBox.height() + 15;
    dialogTextItem->setPos(textX, textY);
    dialogTextItem->setZValue(91); // Above dialogue box
    
    // Set text width to wrap long lines
    QTextDocument* doc = dialogTextItem->document();
    doc->setTextWidth(VIEW_WIDTH - 40); // Allow text to wrap
    
    // Adjust dialog box height if needed for multiline text
    float textHeight = dialogTextItem->boundingRect().height();
    if (textHeight > dialogBox.height() - 30) {
        // If needed, adjust dialog box height
        dialogBoxItem->setScale(textHeight / (dialogBox.height() - 30));
    }

    // Set dialogue as active
    isDialogueActive = true;
}

void TownScene::showDialogue(const QString &text)
{
    // Use the existing showDialogueBox method
    showDialogueBox(text);
}

void TownScene::handleDialogue()
{
    // For now, just close the dialogue when A is pressed
    closeDialogue();
}

void TownScene::closeDialogue()
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

bool TownScene::isPlayerNearBulletinBoard(int &boardIndex) const
{
    // Get player's center position for distance calculation
    QPointF playerCenter(playerPos.x() + 17, playerPos.y() + 30);
    
    // Check each bulletin board
    for (int i = 0; i < bulletinBoardItems.size(); i++) {
        QRectF boardRect = bulletinBoardItems[i]->rect();
        
        // Calculate the center of the bulletin board
        QPointF boardCenter(
            boardRect.x() + boardRect.width()/2,
            boardRect.y() + boardRect.height()/2
        );
        
        // Calculate distance between player and board center
        float dx = playerCenter.x() - boardCenter.x();
        float dy = playerCenter.y() - boardCenter.y();
        float distance = sqrt(dx*dx + dy*dy);
        
        // Check if player is within 25 pixels (circular radius) of the board
        const float INTERACTION_RADIUS = 25.0;
        bool isInRange = (distance <= INTERACTION_RADIUS + boardRect.width()/2);
        
        // Debug output
        qDebug() << "Bulletin board" << i 
                 << "at center:" << boardCenter
                 << "player at:" << playerCenter
                 << "distance:" << distance
                 << "isInRange:" << isInRange;
        
        // Remove the direction check - activate if player is within range regardless of facing direction
        if (isInRange) {
            boardIndex = i;
            return true;
        }
    }
    
    boardIndex = -1;
    return false;
}

bool TownScene::isPlayerNearLabPortal() const
{
    // Get player's feet position (collision box)
    QRectF playerFeet(playerPos.x() + 5, playerPos.y() + 30, 25, 18);
    
    if (!labPortalItem) {
        qDebug() << "Lab portal item is null!";
        return false;
    }
    
    QRectF portalRect = labPortalItem->rect();
    
    // Check if player's feet area intersects with the portal
    bool isOnPortal = playerFeet.intersects(portalRect);
    
    // Debug output
    qDebug() << "Lab portal at:" << portalRect
             << "player feet at:" << playerFeet
             << "isOnPortal:" << isOnPortal;
    
    // Player must be directly on the portal to transport
    return isOnPortal;
}

bool TownScene::isPlayerNearGrasslandPortal() const
{
    // Get player's feet position (collision box)
    QRectF playerFeet(playerPos.x() + 5, playerPos.y() + 30, 25, 18);
    
    if (!grasslandPortalItem) {
        qDebug() << "Grassland portal item is null!";
        return false;
    }
    
    QRectF portalRect = grasslandPortalItem->rect();
    
    // Check if player's feet area intersects with the portal
    bool isOnPortal = playerFeet.intersects(portalRect);
    
    // Debug output
    qDebug() << "Grassland portal at:" << portalRect
             << "player feet at:" << playerFeet
             << "isOnPortal:" << isOnPortal;
    
    // Player must be directly on the portal to transport
    return isOnPortal;
} 
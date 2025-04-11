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
    : Scene(game, scene, parent)
{
    // Create update timer
    updateTimer = new QTimer(this);
    connect(updateTimer, &QTimer::timeout, this, &GrasslandScene::updateScene);
    updateTimer->start(100);  // Update every 100ms
    
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
    createPlayer();

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
    
    // We shouldn't remove or delete scene items here since the scene is managed by Game
    // Just reset our pointers so we don't try to use them later
    backgroundItem = nullptr;
    playerItem = nullptr;
    barrierItems.clear();
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
    
    qDebug() << "Created" << barrierItems.size() << "barriers, 1 town portal, and 1 bulletin board for grassland";
}

void GrasslandScene::handleKeyPress(int key)
{
    qDebug() << "Grassland scene key pressed:" << key;

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
        static int stepCounter = 0;
        stepCounter = 0; // Reset counter when not moving
        return;
    }
    
    QPointF prevPos = playerPos;
    bool moved = false;

    // Speed increases after holding the key for some time
    static int stepCounter = 0;
    int moveSpeed = 8; // Base speed
    
    // Increase speed by 20% after a few steps
    if (stepCounter > 3) {
        moveSpeed = 10; // Increased by 20% from base
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
        }
    } else {
        stepCounter = 0; // Reset counter if no movement happened
    }
}

void GrasslandScene::updateScene()
{
    // If bag is open or dialogue is active, don't update
    if (isBagOpen || isDialogueActive) {
        return;
    }

    // Check if player is on the town portal, and if so, change scene
    if (isPlayerNearTownPortal()) {
        qDebug() << "Player on town portal, switching scene...";
        // Reset movement state before changing scene
        currentPressedKey = 0;
        pressedKeys.clear();
        movementTimer->stop();
        
        // Switch to town scene
        game->changeScene(GameState::TOWN);
        return;
    }

    // This method now just handles general scene updates
    // Movement is handled by processMovement when arrow keys are pressed
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
    
    // Debug output
    qDebug() << "Town portal at:" << portalRect
             << "player feet at:" << playerFeet
             << "isOnPortal:" << isOnPortal;
    
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
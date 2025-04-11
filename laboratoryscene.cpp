#include "laboratoryscene.h"
#include "game.h"
#include <QDebug>
#include <QGraphicsTextItem>
#include <QFont>
#include <QDirIterator>
#include <QApplication>
#include <QGuiApplication>
#include <QTextDocument>

LaboratoryScene::LaboratoryScene(Game *game, QGraphicsScene *scene, QObject *parent)
    : Scene(game, scene, parent)
{
    // Create update timer
    updateTimer = new QTimer(this);
    connect(updateTimer, &QTimer::timeout, this, &LaboratoryScene::updateScene);
    updateTimer->start(100);  // Update every 100ms
    
    // Create movement timer for continuous walking
    movementTimer = new QTimer(this);
    connect(movementTimer, &QTimer::timeout, this, &LaboratoryScene::processMovement);
    movementTimer->setInterval(60);  // smaller number = faster walking speed
}

LaboratoryScene::~LaboratoryScene()
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

void LaboratoryScene::initialize()
{
    qDebug() << "Initializing Laboratory Scene";

    // Debug: print all available resources
    printAvailableResources();

    // Create scene elements
    createBackground();
    createNPC();
    createLabTable();
    createBarriers();
    createPlayer();

    // Set initial camera position to center lab in view
    centerLabInitially();

    // Start update timer
    updateTimer->start(16); // 60 FPS
    // Start movement timer for continuous movement
    movementTimer->start(100);
}

void LaboratoryScene::centerLabInitially()
{
    // Calculate the position to center the lab in the larger scene
    float labOffsetX = (SCENE_WIDTH - LAB_WIDTH) / 2;
    float labOffsetY = (SCENE_HEIGHT - LAB_HEIGHT) / 2;
    
    // The background is already positioned in createBackground, so we don't reposition it here
    
    // Don't reposition NPC and Pokeballs - they're already positioned in their create methods
    // Skip repositioning NPCs and Pokeballs to avoid conflicts
    qDebug() << "centerLabInitially: Using NPC position set in createNPC";
    qDebug() << "centerLabInitially: Using Pokeball positions set in createLabTable";
    
    // Position the player in the lab (with offset)
    playerPos.setX(labOffsetX + 220);
    playerPos.setY(labOffsetY + 350);
    
    if (playerItem) {
        playerItem->setPos(playerPos);
        qDebug() << "Player positioned at:" << playerPos;
    }
    
    // Position barriers
    for (QGraphicsRectItem* barrier : barrierItems) {
        QRectF rect = barrier->rect();
        barrier->setRect(rect.x() + labOffsetX, rect.y() + labOffsetY, rect.width(), rect.height());
    }
    
    // Initial camera setup - center on the player
    updateCamera();
    
    qDebug() << "Laboratory scene initialized with camera following player";
}

void LaboratoryScene::cleanup()
{
    qDebug() << "Cleaning up laboratory scene";
    
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
    npcItem = nullptr;
    labTableItem = nullptr;
    barrierItems.clear();
    pokeBallItems.clear();
    transitionBoxItem = nullptr;
    
    // Don't clear the scene, as it's managed by Game
    qDebug() << "Laboratory scene cleanup complete";
}

void LaboratoryScene::handleKeyPress(int key)
{
    qDebug() << "Lab scene key pressed:" << key;

    // If bag is open, only allow B key to close it
    if (isBagOpen) {
        if (key == Qt::Key_B) {
            toggleBag();
        }
        return; // Block all other key presses while bag is open
    }

    // If dialogue is active, only allow A to advance or number keys for Pokémon selection
    if (isDialogueActive) {
        if (pokemonSelectionActive && (key == Qt::Key_1 || key == Qt::Key_2 || key == Qt::Key_3)) {
            handlePokemonSelection(key);
            return;
        }
        
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
        
        // Take immediate step (5 pixels) - more reliable than 3 pixels
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
            // Calculate the lab offset for boundary checking
            float labOffsetX = (SCENE_WIDTH - LAB_WIDTH) / 2;
            float labOffsetY = (SCENE_HEIGHT - LAB_HEIGHT) / 2;
            
            // Boundary checking - don't allow player to go outside lab area
            if (playerPos.x() < labOffsetX) {
                playerPos.setX(labOffsetX);
            } else if (playerPos.x() > labOffsetX + LAB_WIDTH - 25) {
                playerPos.setX(labOffsetX + LAB_WIDTH - 25);
            }

            if (playerPos.y() < labOffsetY) {
                playerPos.setY(labOffsetY);
            } else if (playerPos.y() > labOffsetY + LAB_HEIGHT - 63) {  // Raised by 10 more pixels (from -53 to -63)
                // Strictly prevent walking outside the lab at the bottom
                playerPos.setY(labOffsetY + LAB_HEIGHT - 63);
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

    // Check for A key to interact with NPCs or objects
    if (key == Qt::Key_A) {
        if (isPlayerNearNPC()) {
            if (!isDialogueActive) {
                currentDialogueState = 0;
                handleDialogue();
            }
        } else if (isPlayerNearDoor()) {
            showDialogue("Would you like to go outside to the town?");
            currentDialogueState = 2; // Special state for door dialogue
        } else {
            int ballIndex = -1;
            if (isPlayerNearPokeball(ballIndex)) {
                startPokemonSelection();
            }
        }
    }
}

void LaboratoryScene::handleKeyRelease(int key)
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

void LaboratoryScene::processMovement()
{
    // Do nothing if no key is pressed or dialogue/bag is open
    if (currentPressedKey == 0 || isDialogueActive || isBagOpen) {
        static int stepCounter = 0;
        stepCounter = 0; // Reset counter when not moving
        return;
    }
    
    QPointF prevPos = playerPos;
    bool moved = false;

    // Calculate the position to center the lab in the larger scene
    float labOffsetX = (SCENE_WIDTH - LAB_WIDTH) / 2;
    float labOffsetY = (SCENE_HEIGHT - LAB_HEIGHT) / 2;

    // Speed increases after holding the key for some time
    static int stepCounter = 0;
    int moveSpeed = 6;
    
    // Increase speed by 50% after a few steps (not 100%)
    if (stepCounter > 3) {
        moveSpeed = 9; // 50% faster (not twice as fast)
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
        // Boundary checking - don't allow player to go outside lab area
        if (playerPos.x() < labOffsetX) {
            playerPos.setX(labOffsetX);
        } else if (playerPos.x() > labOffsetX + LAB_WIDTH - 25) {
            playerPos.setX(labOffsetX + LAB_WIDTH - 25);
        }

        if (playerPos.y() < labOffsetY) {
            playerPos.setY(labOffsetY);
        } else if (playerPos.y() > labOffsetY + LAB_HEIGHT - 58) {  // Changed from -63 to -58 (5 pixels lower)
            // Strictly prevent walking outside the lab at the bottom
            playerPos.setY(labOffsetY + LAB_HEIGHT - 58);
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
            // Check if player is on the transition area
            if (isPlayerOnTransitionArea()) {
                // Reset movement state before changing scene
                currentPressedKey = 0;
                pressedKeys.clear();
                movementTimer->stop();
                
                // Transition to Town scene
                qDebug() << "Player is on transition area - changing to Town scene";
                game->changeScene(GameState::TOWN);
                return;
            }
            
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

void LaboratoryScene::updateScene()
{
    // If bag is open or dialogue is active, don't update
    if (isBagOpen || isDialogueActive) {
        return;
    }

    // This method now just handles general scene updates
    // Movement is handled by processMovement when arrow keys are pressed
}

void LaboratoryScene::createBackground()
{
    // First create a large black background for the entire scene
    QGraphicsRectItem* blackBackground = scene->addRect(0, 0, SCENE_WIDTH, SCENE_HEIGHT, 
        QPen(Qt::transparent), QBrush(Qt::black));
    blackBackground->setZValue(-1);
    qDebug() << "Black background created with size:" << SCENE_WIDTH << "x" << SCENE_HEIGHT;

    // Load laboratory background
    QPixmap background(":/Dataset/Image/scene/lab.png");

    if (background.isNull()) {
        qDebug() << "Laboratory background image not found. Check the path.";
        background = QPixmap(LAB_WIDTH, LAB_HEIGHT);
        background.fill(Qt::white);
    } else {
        qDebug() << "Laboratory background loaded successfully, size:" << background.width() << "x" << background.height();
        
        // Scale the image to match our desired lab dimensions if needed
        if (background.height() < LAB_HEIGHT) {
            background = background.scaled(LAB_WIDTH, LAB_HEIGHT, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
            qDebug() << "Laboratory background scaled to:" << background.width() << "x" << background.height();
        }
    }

    // Calculate the position to center the lab in the larger scene
    float labOffsetX = (SCENE_WIDTH - LAB_WIDTH) / 2;
    float labOffsetY = (SCENE_HEIGHT - LAB_HEIGHT) / 2;

    backgroundItem = scene->addPixmap(background);
    backgroundItem->setPos(labOffsetX, labOffsetY);  // Center the lab in the scene
    backgroundItem->setZValue(0);

    qDebug() << "Laboratory background positioned at:" << labOffsetX << "," << labOffsetY;
    
    // Create a blue transition area at the bottom of the lab that covers only the red door
    // Make it square in shape to cover exactly the red door object
    QRect transitionRect(LAB_WIDTH/2 - 24, LAB_HEIGHT - 37, 55, 38);  
    QGraphicsRectItem* transitionArea = scene->addRect(transitionRect, QPen(Qt::blue, 2), QBrush(QColor(0, 0, 255, 100)));
    transitionArea->setPos(labOffsetX, labOffsetY);
    transitionArea->setZValue(1);
    transitionBoxItem = transitionArea; // Store reference to the transition box
    
    // Make sure the scene background is also black
    scene->setBackgroundBrush(Qt::black);
}

void LaboratoryScene::createNPC()
{
    // Load NPC sprite using the correct path
    QPixmap npcSprite(":/Dataset/Image/NPC.png");
    if (npcSprite.isNull()) {
        qDebug() << "NPC sprite not found at :/Dataset/Image/NPC.png, creating a placeholder";
        // Create a placeholder since the image doesn't exist
        npcSprite = QPixmap(35, 48);
        npcSprite.fill(Qt::blue);
    }
    
    // Calculate the position to center the lab in the larger scene
    float labOffsetX = (SCENE_WIDTH - LAB_WIDTH) / 2;
    float labOffsetY = (SCENE_HEIGHT - LAB_HEIGHT) / 2;
    
    // Position NPC in the upper center of the lab aligned with the barrier at (116,60)
    // Adjusted to better position
    QPointF npcPos(labOffsetX + 195, labOffsetY + 105); // Moved up from 255
    
    npcItem = scene->addPixmap(npcSprite);
    npcItem->setPos(npcPos);
    npcItem->setZValue(3); // Same as player
    qDebug() << "NPC positioned at:" << npcPos;
}

void LaboratoryScene::createPlayer()
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
    playerItem->setPos(playerPos); // Set initial position without camera offset
    playerItem->setZValue(3); // Ensure player is on top of other elements
    qDebug() << "Initial player position:" << playerPos.x() << playerPos.y();
}

void LaboratoryScene::createLabTable()
{
    // Calculate the position to center the lab in the larger scene
    float labOffsetX = (SCENE_WIDTH - LAB_WIDTH) / 2;
    float labOffsetY = (SCENE_HEIGHT - LAB_HEIGHT) / 2;

    // Create Pokeball sprites using the correct path
    QPixmap pokeBallPixmap(":/Dataset/Image/ball.png");
    if (pokeBallPixmap.isNull()) {
        qDebug() << "Pokeball image not found at :/Dataset/Image/ball.png, trying alternative path";
        pokeBallPixmap = QPixmap(":/Dataset/Image/battle/poke_ball.png");
        
        if (pokeBallPixmap.isNull()) {
            qDebug() << "All Pokeball image paths failed, creating a placeholder";
            pokeBallPixmap = QPixmap(20, 20);
            pokeBallPixmap.fill(Qt::red);
        }
    }
    
    // Scale pokeball if needed
    if (pokeBallPixmap.width() > 20 || pokeBallPixmap.height() > 20) {
        pokeBallPixmap = pokeBallPixmap.scaled(20, 20, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    }
    
    // Position the Pokeballs on the table using the barrier coordinates (116,60, 100, 67)
    // Center the balls horizontally on the table with equal spacing
    float tableCenter = labOffsetX + 312; // Center X of table barrier
    float ballSpacing = 30; // Space between balls
    
    // Adjusted to better position
    QPointF ballPos1(tableCenter - ballSpacing, labOffsetY + 185); // Moved up from 220
    QPointF ballPos2(tableCenter, labOffsetY + 185);              // Moved up from 220
    QPointF ballPos3(tableCenter + ballSpacing, labOffsetY + 185); // Moved up from 220
    
    QGraphicsPixmapItem* ball1 = scene->addPixmap(pokeBallPixmap);
    QGraphicsPixmapItem* ball2 = scene->addPixmap(pokeBallPixmap);
    QGraphicsPixmapItem* ball3 = scene->addPixmap(pokeBallPixmap);
    
    ball1->setPos(ballPos1);
    ball2->setPos(ballPos2);
    ball3->setPos(ballPos3);
    
    ball1->setZValue(2);
    ball2->setZValue(2);
    ball3->setZValue(2);
    
    pokeBallItems.append(ball1);
    pokeBallItems.append(ball2);
    pokeBallItems.append(ball3);
    
    qDebug() << "Created pokeballs at:" << ballPos1 << ballPos2 << ballPos3;
}

void LaboratoryScene::createBarriers()
{
    // Calculate the position to center the lab in the larger scene
    float labOffsetX = (SCENE_WIDTH - LAB_WIDTH) / 2;
    float labOffsetY = (SCENE_HEIGHT - LAB_HEIGHT) / 2;
    
    // Define barrier rectangles relative to the lab position
    QVector<QRect> barrierRects = {
        // Left wall area
        QRect(-157, -100, LAB_WIDTH, 90),
        QRect(-157, 10, 30, 90),
        QRect(-125, 32, 70, 105), // red dot machine
        QRect(-157, 215, 170, 80), //left bookshelf
        QRect(118, 215, 170, 80), //right bookshelf
        QRect(-157, 377, 33, 70), //left plant
        QRect(245, 377, 33, 70), //right plant
        QRect(116,60, 100, 67), //pokeball table
        QRect(41, 7, 33, 46), //NPC
    };
    
    // Add barriers (with visible red outlines for debugging)
    for (const QRect &rect : barrierRects) {
        QRect adjustedRect(rect.x() + labOffsetX, rect.y() + labOffsetY, rect.width(), rect.height());
        QGraphicsRectItem *barrier = scene->addRect(adjustedRect, QPen(Qt::red, 1), QBrush(Qt::transparent));
        barrier->setZValue(5); // Higher zValue to be visible for debugging
        barrierItems.append(barrier);
    }
    
    qDebug() << "Created" << barrierItems.size() << "barriers for laboratory at lab offset:" << labOffsetX << "," << labOffsetY;
}

void LaboratoryScene::updatePlayerSprite()
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

void LaboratoryScene::toggleBag()
{
    if (isBagOpen) {
        // Close the bag
        clearBagDisplayItems();
        
        isBagOpen = false;
        qDebug() << "Bag closed";
    } else {
        // Open the bag - use the bag.png image
        isBagOpen = true;
        qDebug() << "Bag opened";
        
        // Update the display to show the Pokémon in the bag
        updateBagDisplay();
    }
}

// Method to clear all bag display items
void LaboratoryScene::clearBagDisplayItems()
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

void LaboratoryScene::updateBagDisplay()
{
    // Always clear previous items before drawing new ones
    clearBagDisplayItems();

    // If bag is not open, do nothing further
    if (!isBagOpen) {
        return;
    }

    // Create bag background image - this should happen regardless of Pokémon
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

    // Display each Pokémon in the bag
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

void LaboratoryScene::updateCamera()
{
    // Don't update camera if player item doesn't exist
    if (!playerItem) return;
    
    // Calculate lab offset
    float labOffsetX = (SCENE_WIDTH - LAB_WIDTH) / 2;
    float labOffsetY = (SCENE_HEIGHT - LAB_HEIGHT) / 2;
    
    // Get the center of the player
    QPointF playerCenter = playerPos + QPointF(17.5, 24); // Center of player sprite
    
    // Calculate desired camera position (centered on player)
    QPointF targetCameraPos;
    targetCameraPos.setX(playerCenter.x() - VIEW_WIDTH / 2);
    targetCameraPos.setY(playerCenter.y() - VIEW_HEIGHT / 2);
    
    // Calculate boundary constraints for the entire scene, not just the lab
    // These define the limits of camera movement ensuring black background is visible on both sides
    float minCameraX = 0; // Left edge of the entire scene
    float maxCameraX = SCENE_WIDTH - VIEW_WIDTH; // Right edge of the entire scene
    float minCameraY = 0; // Top edge of the entire scene
    float maxCameraY = SCENE_HEIGHT - VIEW_HEIGHT; // Bottom edge of the entire scene
    
    // Apply constraints - prevent camera from going outside the entire scene
    if (targetCameraPos.x() < minCameraX) targetCameraPos.setX(minCameraX);
    if (targetCameraPos.y() < minCameraY) targetCameraPos.setY(minCameraY);
    if (targetCameraPos.x() > maxCameraX) targetCameraPos.setX(maxCameraX);
    if (targetCameraPos.y() > maxCameraY) targetCameraPos.setY(maxCameraY);
    
    // Update camera position
    cameraPos = targetCameraPos;
    
    // Update the view
    scene->setSceneRect(cameraPos.x(), cameraPos.y(), VIEW_WIDTH, VIEW_HEIGHT);
    
    // Update dialogue box position if active
    if (isDialogueActive && dialogBoxItem) {
        dialogBoxItem->setPos(cameraPos.x() + 10, cameraPos.y() + VIEW_HEIGHT - 100);
        if (dialogTextItem) {
            dialogTextItem->setPos(cameraPos.x() + 20, cameraPos.y() + VIEW_HEIGHT - 90);
        }
    }
}

void LaboratoryScene::showDialogueBox(const QString &text)
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

void LaboratoryScene::handleDialogue()
{
    // Don't handle dialogue if we're in pokémon selection mode
    if (pokemonSelectionActive) {
        return;
    }
    
    // If no dialogue active, check if we should trigger the NPC dialogue
    if (!isDialogueActive) {
        if (isPlayerNearNPC()) {
            currentDialogueState = 0;
            showDialogue("I am Professor Oak. Welcome to my laboratory!");
        }
        return;
    }
    
    // Handle door dialogue (special state)
    if (currentDialogueState == 2) {
        currentDialogueState++;
        if (currentDialogueState == 3) {
            // Reset movement state before changing scene
            currentPressedKey = 0;
            pressedKeys.clear();
            movementTimer->stop();
            
            // Transition to town scene
            closeDialogue();
            game->changeScene(GameState::TOWN);
            return;
        }
    }
    
    // Advance through dialogue states for NPC
    if (isPlayerNearNPC()) {
        currentDialogueState++;
        
        if (currentDialogueState == 1) {
            showDialogue("You can choose one from three Poké Balls as your initial Pokémon in Laboratory.");
        } else {
            closeDialogue();
        }
        return;
    }
    
    // For other dialogues (like pokémon selection confirmation), just close after one more press
    closeDialogue();
}

void LaboratoryScene::closeDialogue()
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
    pokemonSelectionActive = false;
    currentDialogueState = 0;
}

void LaboratoryScene::showDialogue(const QString &text)
{
    // Use the existing showDialogueBox method
    showDialogueBox(text);
}

bool LaboratoryScene::isPlayerNearNPC() const
{
    // Calculate the position to center the lab in the larger scene
    float labOffsetX = (SCENE_WIDTH - LAB_WIDTH) / 2;
    float labOffsetY = (SCENE_HEIGHT - LAB_HEIGHT) / 2;
    
    // Use the same NPC position as in createNPC method
    QPointF npcPos(labOffsetX + 195, labOffsetY + 105);
    
    // Create a larger detection area BELOW the NPC, not offset by y+40
    // This allows the player to stand in front of the NPC and interact
    QRectF npcArea(npcPos.x() - 40, npcPos.y() + 10, 80, 60);
    
    // Check if player is within the designated area
    bool isInRange = npcArea.contains(playerPos);
    bool isFacingNPC = playerDirection == "B";  // Facing up toward the NPC
    
    qDebug() << "isPlayerNearNPC check: Player at" << playerPos << "NPC at" << npcPos 
             << "Area:" << npcArea << "isInRange:" << isInRange << "isFacingNPC:" << isFacingNPC;
    
    return isInRange && isFacingNPC;
}

void LaboratoryScene::printAvailableResources()
{
    qDebug() << "Checking for critical resources:";
    
    // Pokemon images
    QStringList criticalImages = {
        ":/Dataset/Image/battle/charmander.png",
        ":/Dataset/Image/battle/squirtle.png",
        ":/Dataset/Image/battle/bulbasaur.png",
        ":/Dataset/Image/bag.png",
        ":/Dataset/Image/dialog.png",
        ":/Dataset/Image/ball.png"
    };
    
    for (const QString& path : criticalImages) {
        QPixmap img(path);
        if (img.isNull()) {
            qDebug() << "RESOURCE MISSING:" << path;
        } else {
            qDebug() << "Resource found:" << path;
        }
    }
}

bool LaboratoryScene::checkCollision()
{
    // Boundary checking
    if (playerPos.x() < 0 || playerPos.x() > LAB_WIDTH - 35 ||
        playerPos.y() < 0 || playerPos.y() > LAB_HEIGHT - 48) {
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

void LaboratoryScene::updatePlayerPosition()
{
    if (playerItem) {
        playerItem->setPos(playerPos);
        // Camera will follow player
        updateCamera();
    }
}

bool LaboratoryScene::isPlayerNearPokeball(int &ballIndex) const
{
    // Calculate the position to center the lab in the larger scene
    float labOffsetX = (SCENE_WIDTH - LAB_WIDTH) / 2;
    float labOffsetY = (SCENE_HEIGHT - LAB_HEIGHT) / 2;
    
    // Create a detection area covering the table and area in front, 20% smaller than before
    // Adjust for the lab offset
    float width = 150 * 0.8;    // 20% smaller width (changed from 0.85)
    float height = 100 * 0.8;   // 20% smaller height (changed from 0.85)
    // Adjust starting position to keep centered
    float x = labOffsetX + 258;  // Aligned with the table barrier
    float y = labOffsetY + 170;  // Just below the table barrier
    QRectF tableArea(x, y, width, height);
    
    // Check if player is within the designated area
    if (tableArea.contains(playerPos)) {
        ballIndex = 0; // Placeholder, actual selection will be done with number keys
        return true;
    }
    
    ballIndex = -1;
    return false;
}

void LaboratoryScene::startPokemonSelection()
{
    // Check if player has already chosen a starter Pokémon
    if (hasChosenPokemon) {
        // Show message that they've already chosen
        showDialogue("You have already chosen your starter Pokémon.");
        
        // We're not entering Pokémon selection mode, just showing a message
        pokemonSelectionActive = false;
        return;
    }
    
    // If they haven't chosen yet, proceed with selection
    showDialogue("Choose your Pokemon: Press 1 for Squirtle, 2 for Charmander, or 3 for Bulbasaur.");
    
    // Set flag to indicate we're in Pokémon selection mode
    pokemonSelectionActive = true;
}

void LaboratoryScene::handlePokemonSelection(int key)
{
    int selectedIndex = -1;
    
    // Convert key to selection index
    if (key == Qt::Key_1) {
        qDebug() << "Player selected Squirtle";
        selectedIndex = 0; // Squirtle
    }
    else if (key == Qt::Key_2) {
        qDebug() << "Player selected Charmander";
        selectedIndex = 1; // Charmander
    }
    else if (key == Qt::Key_3) {
        qDebug() << "Player selected Bulbasaur";
        selectedIndex = 2; // Bulbasaur
    }
    else if (key == Qt::Key_Escape) {
        qDebug() << "Player cancelled pokémon selection";
        closeDialogue();
        pokemonSelectionActive = false;
        return;
    }
    else {
        return; // Invalid key, do nothing
    }
    
    // Process the selection
    choosePokemon(selectedIndex);
}

void LaboratoryScene::choosePokemon(int pokemonIndex)
{
    // Map the selection numbers to the correct Pokémon types
    Pokemon::Type type;
    QString pokemonName;
    
    switch (pokemonIndex) {
        case 0:
            type = Pokemon::SQUIRTLE;
            pokemonName = "Squirtle";
            break;
        case 1:
            type = Pokemon::CHARMANDER;
            pokemonName = "Charmander";
            break;
        case 2:
            type = Pokemon::BULBASAUR;
            pokemonName = "Bulbasaur";
            break;
        default:
            qDebug() << "Invalid Pokémon index:" << pokemonIndex;
            return;
    }
    
    // Create a new Pokémon and add it to player's collection
    Pokemon* selectedPokemon = new Pokemon(type);
    if (!selectedPokemon) {
        qDebug() << "ERROR: Failed to create Pokémon of type" << static_cast<int>(type);
        return;
    }
    
    qDebug() << "Created a new Pokémon:" << selectedPokemon->getName() << "with image path:" << selectedPokemon->getImagePath();
    
    // Add Pokémon to player's collection
    game->addPokemon(selectedPokemon);
    
    // Verify the Pokémon was added successfully
    const QVector<Pokemon*>& playerPokemon = game->getPokemon();
    qDebug() << "After adding, player now has" << playerPokemon.size() << "Pokémon";
    
    if (!playerPokemon.isEmpty()) {
        qDebug() << "First Pokémon in collection is:" << playerPokemon[0]->getName();
    }
    
    // Show a confirmation message
    showDialogue("You chose " + pokemonName + " as your partner!");
    
    // Set flag to indicate player has chosen a Pokémon
    hasChosenPokemon = true;
    
    // Reset pokémon selection state but keep dialogue active for confirmation
    pokemonSelectionActive = false;
    
    // Remove all pokeball sprites since the player has made their choice
    for (auto ball : pokeBallItems) {
        if (ball) {
            scene->removeItem(ball);
            delete ball;
        }
    }
    pokeBallItems.clear();
    
    qDebug() << "Pokémon selection complete. Press A to close dialogue, then B to open your bag.";
}

// New method to check if player is near the laboratory door
bool LaboratoryScene::isPlayerNearDoor() const
{
    // Calculate the position to center the lab in the larger scene
    float labOffsetX = (SCENE_WIDTH - LAB_WIDTH) / 2;
    float labOffsetY = (SCENE_HEIGHT - LAB_HEIGHT) / 2;
    
    // Define door area (center bottom of lab)
    QRectF doorArea(labOffsetX + LAB_WIDTH/2 - 30, labOffsetY + LAB_HEIGHT - 60, 60, 20);
    
    // Check if player is within the door area and facing down
    bool isInRange = doorArea.contains(playerPos);
    bool isFacingDoor = playerDirection == "F";
    
    return isInRange && isFacingDoor;
}

bool LaboratoryScene::isPlayerOnTransitionArea() const
{
    if (!transitionBoxItem) {
        qDebug() << "Transition box not created!";
        return false;
    }
    
    // Calculate the position to center the lab in the larger scene
    float labOffsetX = (SCENE_WIDTH - LAB_WIDTH) / 2;
    float labOffsetY = (SCENE_HEIGHT - LAB_HEIGHT) / 2;
    
    // Get the transition box rect (this rect already has the lab offset applied)
    QRectF transitionRect = transitionBoxItem->rect();
    
    // Adjust for the position of the box
    QRectF adjustedRect(
        transitionRect.x() + transitionBoxItem->pos().x(),
        transitionRect.y() + transitionBoxItem->pos().y(),
        transitionRect.width(), 
        transitionRect.height()
    );
    
    // Use the player's feet position for detection
    QPointF playerFeet(playerPos.x() + 17, playerPos.y() + 40);
    
    // Check if player is inside the transition area
    bool isInTransitionArea = adjustedRect.contains(playerFeet);
    
    qDebug() << "Transition check: Player at" << playerFeet 
             << "Transition area:" << adjustedRect
             << "Result:" << isInTransitionArea
             << "hasChosenPokemon:" << hasChosenPokemon;
    
    return isInTransitionArea;
}


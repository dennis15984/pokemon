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
    
    // Adjust initial positions of all objects to be relative to lab's position
    if (npcItem) {
        npcItem->setPos(labOffsetX + 220, labOffsetY + 65); // Moved down by 15 pixels from original 50
        qDebug() << "NPC positioned at:" << npcItem->pos();
    }
    
    // Position the player in the lab (with offset)
    playerPos.setX(labOffsetX + 220);
    playerPos.setY(labOffsetY + 350);
    
    if (playerItem) {
        playerItem->setPos(playerPos);
        qDebug() << "Player positioned at:" << playerPos;
    }
    
    // Position pokeballs
    for (int i = 0; i < pokeBallItems.size(); i++) {
        if (pokeBallItems[i]) {
            pokeBallItems[i]->setPos(labOffsetX + (280 + i * 35), labOffsetY + 130);
        }
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
    // Stop update timer
    updateTimer->stop();

    // Clear bag display items explicitly
    clearBagDisplayItems();

    // Clear scene
    scene->clear();

    // Reset pointers
    backgroundItem = nullptr;
    playerItem = nullptr;
    npcItem = nullptr;
    labTableItem = nullptr;
    pokeBallItems.clear();
    barrierItems.clear();
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
            
            // Boundary checks
            if (playerPos.x() < labOffsetX) {
                playerPos.setX(labOffsetX);
            } else if (playerPos.x() > labOffsetX + LAB_WIDTH - 25) {
                playerPos.setX(labOffsetX + LAB_WIDTH - 25);
            }

            if (playerPos.y() < labOffsetY) {
                playerPos.setY(labOffsetY);
            } else if (playerPos.y() > labOffsetY + LAB_HEIGHT - 48) {
                playerPos.setY(labOffsetY + LAB_HEIGHT - 48);
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
        } else if (playerPos.y() > labOffsetY + LAB_HEIGHT - 48) {
            playerPos.setY(labOffsetY + LAB_HEIGHT - 48);
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
    }

    // Calculate the position to center the lab in the larger scene
    float labOffsetX = (SCENE_WIDTH - LAB_WIDTH) / 2;
    float labOffsetY = (SCENE_HEIGHT - LAB_HEIGHT) / 2;

    backgroundItem = scene->addPixmap(background);
    backgroundItem->setPos(labOffsetX, labOffsetY);  // Center the lab in the scene
    backgroundItem->setZValue(0);

    qDebug() << "Laboratory background positioned at:" << labOffsetX << "," << labOffsetY;
    
    // Make sure the scene background is also black
    scene->setBackgroundBrush(Qt::black);
}

void LaboratoryScene::createNPC()
{
    // Load NPC (Professor Oak) sprite
    QPixmap npcSprite(":/Dataset/Image/NPC.png");

    if (npcSprite.isNull()) {
        qDebug() << "NPC sprite not found at :/Dataset/Image/NPC.png, creating a placeholder";
        npcSprite = QPixmap(35, 48);
        npcSprite.fill(Qt::blue);
    } else {
        qDebug() << "NPC sprite loaded successfully";
    }

    npcItem = scene->addPixmap(npcSprite);
    npcItem->setPos(220, 65); // Moved down by 15 pixels from original 50
    npcItem->setZValue(2);
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
    QPixmap ballSprite(":/Dataset/Image/ball.png");

    if (ballSprite.isNull()) {
        qDebug() << "Poké Ball sprite not found at :/Dataset/Image/ball.png, creating placeholders";
        ballSprite = QPixmap(20, 20);
        ballSprite.fill(Qt::red);
    } else {
        qDebug() << "Poké Ball sprite loaded successfully";
    }

    // Add three Poké Balls in a row - positioned on the table in the background
    for (int i = 0; i < 3; i++) {
        QGraphicsPixmapItem *ball = scene->addPixmap(ballSprite);
        ball->setPos((280 + i * 35), 130); // Increased spacing between balls to 35 pixels
        ball->setZValue(1); // Just above background
        pokeBallItems.append(ball);
    }
}

void LaboratoryScene::createBarriers()
{
    // Define barriers based on the visible objects in the lab (512x512 pixels)
    QVector<QRect> barrierRects = {
        // Top wall equipment and furniture (extend full width)
        QRect(0, 0, 438, 75),
        
        // Bottom left furniture (bookcases)
        QRect(0, 253, 180, 60),
        
        // Bottom right furniture (bookcases)
        QRect(283, 253, 180, 60),
        
        QRect(0, 90, 30, 65),  // Top computer equipment
       
        // Machine with red button (center left)
        QRect(40, 112, 68, 72),
        
        // Plants in bottom corners
        QRect(0, 372, 30, 63),
        QRect(420, 372, 30, 63), 

        QRect(282, 125, 100, 52), //pokeball table
        QRect(220, 63, 35, 45), // npc
    };

    // Add barriers (with visible red outlines for debugging)
    for (const QRect &rect : barrierRects) {
        QGraphicsRectItem *barrier = scene->addRect(rect, QPen(Qt::red, 1), QBrush(Qt::transparent));
        barrier->setZValue(5); // Higher zValue to be visible for debugging
        barrierItems.append(barrier);
    }
    
    // For debugging - print out all barriers
    qDebug() << "Created" << barrierItems.size() << "barriers";
    for (int i = 0; i < barrierItems.size(); i++) {
        qDebug() << "Barrier" << i << ":" << barrierItems[i]->rect();
    }
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
    
    // Start at a higher position for first row - adjust for proper alignment
    float startY = bagY + 5; // Reduced from 10 to 5 for better vertical alignment
    
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

void LaboratoryScene::updateCamera()
{
    // Make sure playerItem exists
    if (!playerItem) return;
    
    // Calculate the position to center the lab in the larger scene
    float labOffsetX = (SCENE_WIDTH - LAB_WIDTH) / 2;
    float labOffsetY = (SCENE_HEIGHT - LAB_HEIGHT) / 2;
    
    // Get player position (center of player)
    QPointF playerCenter = playerPos + QPointF(17.5, 24); // Center of the player sprite
    
    // Calculate camera position to center on the player
    cameraPos.setX(playerCenter.x() - VIEW_WIDTH / 2);
    cameraPos.setY(playerCenter.y() - VIEW_HEIGHT / 2);
    
    // Constrain camera to stay within the scene bounds
    // Don't let the camera show areas outside the scene
    if (cameraPos.x() < 0) cameraPos.setX(0);
    if (cameraPos.y() < 0) cameraPos.setY(0);
    if (cameraPos.x() > SCENE_WIDTH - VIEW_WIDTH) cameraPos.setX(SCENE_WIDTH - VIEW_WIDTH);
    if (cameraPos.y() > SCENE_HEIGHT - VIEW_HEIGHT) cameraPos.setY(SCENE_HEIGHT - VIEW_HEIGHT);
    
    // Update the view position to follow the player
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
    
    // Get NPC position with lab offset
    QPointF npcPos(labOffsetX + 220, labOffsetY + 65);
    
    // Calculate horizontal and vertical distances separately
    double horizontalDist = qAbs(playerPos.x() - npcPos.x());
    double verticalDist = playerPos.y() - npcPos.y();
    
    // Debug NPC interaction check
    qDebug() << "NPC check - horizontalDist:" << horizontalDist << "verticalDist:" << verticalDist 
             << "playerDirection:" << playerDirection;
    
    // Player should be facing up (B) and standing below the NPC within interaction range
    bool isInRange = horizontalDist < 25 && verticalDist > 0 && verticalDist < 50;
    bool isFacingNPC = playerDirection == "B" && playerPos.y() > npcPos.y();
    
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
    // Create a detection area covering the table and area in front, 15% smaller than before
    // Original was: QRectF tableArea(250, 125, 150, 100);
    float width = 150 * 0.85;    // 15% smaller width
    float height = 100 * 0.85;   // 15% smaller height
    // Adjust starting position to keep centered
    float x = 250 + (150 - width) / 2;
    float y = 125 + (100 - height) / 2;
    QRectF tableArea(x, y, width, height);
    
    // Calculate lab offset
    float labOffsetX = (SCENE_WIDTH - LAB_WIDTH) / 2;
    float labOffsetY = (SCENE_HEIGHT - LAB_HEIGHT) / 2;
    
    // Adjust table area with lab offset
    tableArea.translate(labOffsetX, labOffsetY);
    
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


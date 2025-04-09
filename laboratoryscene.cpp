#include "laboratoryscene.h"
#include "game.h"
#include <QDebug>
#include <QGraphicsTextItem>
#include <QFont>
#include <QDirIterator>

LaboratoryScene::LaboratoryScene(Game *game, QGraphicsScene *scene, QObject *parent)
    : Scene(game, scene, parent),
      backgroundItem(nullptr),
      playerItem(nullptr),
      npcItem(nullptr),
      labTableItem(nullptr),
      transitionPoint(nullptr),
      transitionNumberText(nullptr),
      transitionNumberBg(nullptr),
      updateTimer(new QTimer(this)),
      playerPos(220, 350), // Adjusted initial player position to be higher up
      cameraPos(0, 0)      // Initial camera position
{
    connect(updateTimer, &QTimer::timeout, this, &LaboratoryScene::updateScene);
}

LaboratoryScene::~LaboratoryScene()
{
    cleanup();
    delete updateTimer;
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
    createTransitionPoint();

    // Set initial camera position
    updateCamera();

    // Start update timer
    updateTimer->start(16); // 60 FPS
}

void LaboratoryScene::cleanup()
{
    // Stop update timer
    updateTimer->stop();

    // Clear scene
    scene->clear();

    // Reset pointers
    backgroundItem = nullptr;
    playerItem = nullptr;
    npcItem = nullptr;
    labTableItem = nullptr;
    pokeBallItems.clear();
    barrierItems.clear();
    transitionPoint = nullptr;
    transitionNumberText = nullptr;
    transitionNumberBg = nullptr;
}

void LaboratoryScene::handleKeyPress(int key)
{
    // Store the previous position to check for collisions
    QPointF prevPos = playerPos;

    // Movement keys
    if (key == Qt::Key_Up || key == Qt::Key_W) {
        playerPos.setY(playerPos.y() - 5);
    } else if (key == Qt::Key_Down || key == Qt::Key_S) {
        playerPos.setY(playerPos.y() + 5);
    } else if (key == Qt::Key_Left || key == Qt::Key_A) {
        playerPos.setX(playerPos.x() - 5);
    } else if (key == Qt::Key_Right || key == Qt::Key_D) {
        playerPos.setX(playerPos.x() + 5);
    }

    // Interaction key
    else if (key == Qt::Key_Return || key == Qt::Key_E) {
        // Check if near NPC for dialogue
        if (npcItem && qAbs(playerPos.x() - (320 + cameraPos.x())) < 50 &&
            qAbs(playerPos.y() - (130 + cameraPos.y())) < 50) {
            showDialogue("I am Professor Oak. Welcome to my laboratory!\nYou can choose one from three Poké Balls as your initial Pokémon.");
        }

        // Check if near lab table for Pokémon selection
        else if (labTableItem && qAbs(playerPos.x() - (320 + cameraPos.x())) < 50 &&
                qAbs(playerPos.y() - (175 + cameraPos.y())) < 50) {
            // In future implementation, this will open a Pokémon selection UI
            showDialogue("Choose your starter Pokémon!");
        }

        // Check if at transition point
        else if (transitionPoint && qAbs(playerPos.x() - (220 + cameraPos.x())) < 30 &&
                qAbs(playerPos.y() - (430 + cameraPos.y())) < 30) {
            game->changeScene(GameState::TOWN);
        }
    }

    // Simple boundary checking
    if (playerPos.x() < 0 || playerPos.x() > LAB_WIDTH - 35 ||
        playerPos.y() < 0 || playerPos.y() > LAB_HEIGHT - 48) {
        playerPos = prevPos; // Revert to previous position if out of bounds
    }

    // Check collision with barriers (simple implementation)
    for (const QGraphicsRectItem* barrier : barrierItems) {
        QRectF playerRect(playerPos.x(), playerPos.y(), 35, 48);
        if (playerRect.intersects(barrier->rect())) {
            playerPos = prevPos; // Revert to previous position if collision
            break;
        }
    }

    // Update player position and camera
    if (playerItem) {
        playerItem->setPos(playerPos - cameraPos);
        updateCamera();
    }
}

void LaboratoryScene::updateScene()
{
    // This will be used for animations and continuous updates
}

void LaboratoryScene::createBackground()
{
    // Load laboratory background
    QPixmap background(":/Dataset/Image/scene/lab.png");

    if (background.isNull()) {
        qDebug() << "Laboratory background image not found. Check the path.";
        background = QPixmap(LAB_WIDTH, LAB_HEIGHT);
        background.fill(Qt::white);
    } else {
        qDebug() << "Laboratory background loaded successfully";
    }

    backgroundItem = scene->addPixmap(background);
    backgroundItem->setZValue(0);

    // Black background to fill the window
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
    npcItem->setPos(280 - cameraPos.x(), 100 - cameraPos.y()); // Moved NPC more left and up
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
    playerItem->setPos(playerPos - cameraPos); // Position adjusted for camera
    playerItem->setZValue(3); // Ensure player is on top of other elements
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
    // Create barriers for lab equipment, shelves, etc.
    QList<QRect> barrierRects = {
        // Top shelves and equipment (the whole top section)
        QRect(0, 0, LAB_WIDTH, 100),

        // Left equipment (the machine with red button)
        QRect(0, 100, 120, 100),

        // Bottom shelves
        QRect(140, 280, 160, 60),
        QRect(340, 280, 160, 60),

        // Side barriers (to prevent going off the edges)
        QRect(0, 0, 10, LAB_HEIGHT),
        QRect(LAB_WIDTH - 10, 0, 10, LAB_HEIGHT)
    };

    // Add barriers (invisible in the final game)
    for (const QRect &rect : barrierRects) {
        QGraphicsRectItem *barrier = scene->addRect(rect, QPen(Qt::transparent), QBrush(Qt::transparent));
        barrier->setZValue(1);
        barrier->setPos(-cameraPos.x(), -cameraPos.y()); // Adjust for camera
        barrierItems.append(barrier);
    }
}

void LaboratoryScene::createTransitionPoint()
{
    // Create transition area at the bottom of the lab
    QPixmap transitionSprite(":/Dataset/Image/element/transition.png");

    if (transitionSprite.isNull()) {
        // Create a blue rectangle for the transition point
        transitionSprite = QPixmap(50, 20);
        transitionSprite.fill(QColor(0, 0, 255, 100)); // Semi-transparent blue
    }

    transitionPoint = scene->addPixmap(transitionSprite);
    transitionPoint->setPos(220 - cameraPos.x(), 430 - cameraPos.y()); // Bottom center, adjusted for camera
    transitionPoint->setZValue(1);

    // Create blue background rectangle for number
    transitionNumberBg = scene->addRect(0, 0, 25, 25,
                                    QPen(Qt::transparent),
                                    QBrush(QColor(65, 105, 225))); // Royal Blue
    transitionNumberBg->setPos((220 - 12.5) - cameraPos.x(), (430 - 12.5) - cameraPos.y());
    transitionNumberBg->setZValue(2);

    // Add a text box with "1" on the transition point
    transitionNumberText = scene->addText("1");
    transitionNumberText->setDefaultTextColor(Qt::white);

    QFont numberFont("Arial", 14, QFont::Bold);
    transitionNumberText->setFont(numberFont);

    // Center the "1" on the blue background
    QRectF textRect = transitionNumberText->boundingRect();
    transitionNumberText->setPos(
        (220 - textRect.width()/2) - cameraPos.x(),
        (430 - textRect.height()/2) - cameraPos.y()
    );
    transitionNumberText->setZValue(3);
}

void LaboratoryScene::updateCamera()
{
    // Calculate camera position to keep player centered
    cameraPos.setX(playerPos.x() - (VIEW_WIDTH - 35) / 2);
    cameraPos.setY(playerPos.y() - (VIEW_HEIGHT - 48) / 2);

    // Limit camera to lab boundaries
    cameraPos.setX(qMax(0.0, qMin(cameraPos.x(), (double)LAB_WIDTH - VIEW_WIDTH)));
    cameraPos.setY(qMax(0.0, qMin(cameraPos.y(), (double)LAB_HEIGHT - VIEW_HEIGHT)));

    // Update all visible objects based on camera position
    if (backgroundItem) backgroundItem->setPos(-cameraPos);
    if (npcItem) npcItem->setPos(280 - cameraPos.x(), 100 - cameraPos.y()); // Updated NPC position to match createNPC
    if (playerItem) playerItem->setPos(playerPos - cameraPos);
    if (transitionPoint) transitionPoint->setPos(220 - cameraPos.x(), 430 - cameraPos.y());

    // Update transition number
    if (transitionNumberBg) {
        transitionNumberBg->setPos((220 - 12.5) - cameraPos.x(), (430 - 12.5) - cameraPos.y());
    }
    if (transitionNumberText) {
        QRectF textRect = transitionNumberText->boundingRect();
        transitionNumberText->setPos(
            (220 - textRect.width()/2) - cameraPos.x(),
            (430 - textRect.height()/2) - cameraPos.y()
        );
    }

    // Update Poké Balls
    for (int i = 0; i < pokeBallItems.size(); i++) {
        if (pokeBallItems[i]) {
            pokeBallItems[i]->setPos((280 + i * 35) - cameraPos.x(), 130 - cameraPos.y());
        }
    }

    // Update barriers
    for (int i = 0; i < barrierItems.size(); i++) {
        barrierItems[i]->setPos(-cameraPos);
    }
}

void LaboratoryScene::showDialogue(const QString &text)
{
    // Simple dialogue implementation
    QGraphicsRectItem *dialogBox = scene->addRect(10, 350, 505, 80, QPen(Qt::black), QBrush(QColor(255, 255, 255, 220)));
    dialogBox->setZValue(10);

    QGraphicsTextItem *dialogText = scene->addText(text);
    dialogText->setDefaultTextColor(Qt::black);
    dialogText->setPos(20, 360);
    dialogText->setZValue(11);

    QFont dialogFont("Arial", 10);
    dialogText->setFont(dialogFont);

    // Dialogue disappears after 3 seconds
    QTimer::singleShot(3000, [this, dialogBox, dialogText]() {
        scene->removeItem(dialogBox);
        scene->removeItem(dialogText);
        delete dialogBox;
        delete dialogText;
    });
}

void LaboratoryScene::printAvailableResources()
{
    qDebug() << "Listing all available resources:";
    QDirIterator it(":/", QDirIterator::Subdirectories);
    while (it.hasNext()) {
        QString resource = it.next();
        qDebug() << "Resource:" << resource;
    }
}

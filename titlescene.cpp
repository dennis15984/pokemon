#include "titlescene.h"
#include "game.h"
#include <QGraphicsScene>
#include <QPixmap>
#include <QFont>
#include <QColor>
#include <QDebug>
#include <QGraphicsTextItem>

TitleScene::TitleScene(Game *game, QGraphicsScene *scene, QObject *parent)
    : Scene(game, scene, parent),
      backgroundItem(nullptr),
      titleTextItem(nullptr),
      pressStartTextItem(nullptr),
      textBackgroundItem(nullptr),
      blinkTimer(new QTimer(this)),
      textVisible(true)
{
    // Connect blink timer
    connect(blinkTimer, &QTimer::timeout, this, &TitleScene::blinkPressStartText);
}

TitleScene::~TitleScene()
{
    cleanup();
    if (blinkTimer) {
        blinkTimer->stop();
        delete blinkTimer;
    }
}

void TitleScene::initialize()
{
    qDebug() << "Initializing Title Scene";
    
    // Create scene elements
    createBackground();
    createTitleText();
    
    // Center the camera on the scene
    centerCamera();
    
    // Start blinking animation for "Press Start" text
    blinkTimer->start(500); // Blink every 500ms
}

void TitleScene::cleanup()
{
    if (blinkTimer) {
        blinkTimer->stop();
    }
    
    // Clear all items from the scene
    scene->clear();
    
    // Reset pointers
    backgroundItem = nullptr;
    titleTextItem = nullptr;
    pressStartTextItem = nullptr;
    textBackgroundItem = nullptr;
}

void TitleScene::handleKeyPress(int key)
{
    qDebug() << "Title scene key pressed:" << key;

    // Only respond to Enter/Return key
    if (key == Qt::Key_Return || key == Qt::Key_Enter) {
        qDebug() << "Starting game...";
        emit startGame();
    }
}

void TitleScene::createBackground()
{
    // Create a background that exactly matches the window size (525x450)
    QPixmap bgPixmap(":/Dataset/Image/scene/start_menu.png");
    
    if (bgPixmap.isNull()) {
        qDebug() << "Title background image not found, creating a black background";
        bgPixmap = QPixmap(TITLE_WIDTH, TITLE_HEIGHT);
        bgPixmap.fill(Qt::black);
    } else {
        // Scale the image to exactly match the window size
        bgPixmap = bgPixmap.scaled(TITLE_WIDTH, TITLE_HEIGHT, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
        qDebug() << "Title background loaded and scaled to:" << TITLE_WIDTH << "x" << TITLE_HEIGHT;
    }
    
    backgroundItem = scene->addPixmap(bgPixmap);
    backgroundItem->setPos(0, 0);  // Position at exact top-left corner
    backgroundItem->setZValue(0);
    
    // Make sure the scene background is also black
    scene->setBackgroundBrush(Qt::black);
}

void TitleScene::createTitleText()
{
    // Create a white background for the text for better visibility
    textBackgroundItem = scene->addRect(
        0, 0, 300, 50,
        QPen(Qt::transparent),
        QBrush(QColor(255, 255, 255, 200)) // Semi-transparent white background
    );
    
    // Only add "Press Start" text
    pressStartTextItem = scene->addText("Press Enter to Start", QFont("Arial", 22, QFont::Bold));
    pressStartTextItem->setDefaultTextColor(Qt::black);
    
    // Position both the background and text
    QRectF textRect = pressStartTextItem->boundingRect();
    int textX = (TITLE_WIDTH - textRect.width()) / 2;
    int textY = TITLE_HEIGHT * 2 / 3;
    
    textBackgroundItem->setPos(textX - 10, textY - 5); // Position with some padding
    textBackgroundItem->setRect(0, 0, textRect.width() + 20, textRect.height() + 10);
    textBackgroundItem->setZValue(1);
    
    pressStartTextItem->setPos(textX, textY);
    pressStartTextItem->setZValue(2); // Above the background
}

void TitleScene::centerCamera()
{
    // Set the view to exactly the title screen size
    scene->setSceneRect(0, 0, TITLE_WIDTH, TITLE_HEIGHT);
    
    // Position elements directly for the title screen
    if (backgroundItem) backgroundItem->setPos(0, 0);
    
    if (pressStartTextItem && textBackgroundItem) {
        QRectF textRect = pressStartTextItem->boundingRect();
        int textX = (TITLE_WIDTH - textRect.width()) / 2;
        int textY = TITLE_HEIGHT * 2 / 3;
        
        pressStartTextItem->setPos(textX, textY);
        textBackgroundItem->setPos(textX - 10, textY - 5);
        textBackgroundItem->setRect(0, 0, textRect.width() + 20, textRect.height() + 10);
    }
    
    qDebug() << "Title scene camera positioned at 0,0 with size" << TITLE_WIDTH << "x" << TITLE_HEIGHT;
}

void TitleScene::blinkPressStartText()
{
    textVisible = !textVisible;
    if (pressStartTextItem) {
        pressStartTextItem->setVisible(textVisible);
        // Keep the background always visible for better visibility
        if (textBackgroundItem) {
            textBackgroundItem->setVisible(true);
        }
    }
}

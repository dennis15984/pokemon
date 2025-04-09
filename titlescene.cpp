#include "titlescene.h"
#include "game.h"
#include <QGraphicsScene>
#include <QPixmap>
#include <QFont>
#include <QColor>
#include <QDebug>

TitleScene::TitleScene(Game *game, QGraphicsScene *scene, QObject *parent)
    : Scene(game, scene, parent),
      backgroundItem(nullptr),
      logoItem(nullptr),
      startTextItem(nullptr),
      animationTimer(new QTimer(this)),
      animationFrame(0),
      startTextVisible(true)
{
    // Connect animation timer
    connect(animationTimer, &QTimer::timeout, this, &TitleScene::animateTitle);
}

TitleScene::~TitleScene()
{
    cleanup();
    delete animationTimer;
}

void TitleScene::initialize()
{
    qDebug() << "Initializing Title Scene";

    // Create scene elements
    createBackground();
    createLogo();
    createStartText();

    // Start animation timer
    animationTimer->start(500); // Update every 500ms
}

void TitleScene::cleanup()
{
    // Stop animation timer
    animationTimer->stop();

    // Clear scene
    scene->clear();

    // Reset pointers
    backgroundItem = nullptr;
    logoItem = nullptr;
    startTextItem = nullptr;
}

void TitleScene::handleKeyPress(int key)
{
    // Enter key (or Return) starts the game
    if (key == Qt::Key_Return || key == Qt::Key_Enter) {
        qDebug() << "Starting game from title screen";
        game->changeScene(GameState::LABORATORY);
    }
}

void TitleScene::animateTitle()
{
    // Simple animation to make the "Press Enter to Start" text blink
    startTextVisible = !startTextVisible;
    if (startTextItem) {
        startTextItem->setVisible(startTextVisible);
    }

    // Animate logo (could add a slight bounce or glow effect)
    animationFrame = (animationFrame + 1) % 2;
    if (logoItem) {
        // Simple scaling effect for the logo
        qreal scale = 1.0 + (animationFrame * 0.05);
        logoItem->setScale(scale);
    }
}

void TitleScene::createBackground()
{
    // Try to load background image using the correct path
    QPixmap background(":/Dataset/Image/scene/start_menu.png");

    // If image not found, create a simple colored background
    if (background.isNull()) {
        qDebug() << "Title background image not found, creating a default background";
        background = QPixmap(525, 450);
        background.fill(QColor(135, 206, 235)); // Sky blue color
    }

    // Create and add the background item
    backgroundItem = scene->addPixmap(background);
    backgroundItem->setZValue(0); // Ensure background is at the bottom layer
}

void TitleScene::createLogo()
{
    // Since we're using a background image that already has the logo,
    // we don't need to add a separate logo or text
}

void TitleScene::createStartText()
{
    // Create "Press Enter to Start" text
    startTextItem = scene->addText("Press Enter to Start");
    startTextItem->setDefaultTextColor(Qt::white);

    QFont startFont("Arial", 18);
    startTextItem->setFont(startFont);

    // Position the text at the bottom of the screen
    QRectF textRect = startTextItem->boundingRect();
    startTextItem->setPos((525 - textRect.width()) / 2, 350);
    startTextItem->setZValue(2);
}

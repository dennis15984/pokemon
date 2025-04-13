#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QKeyEvent>
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QPainter>
#include <QMenuBar>
#include <QStatusBar>
#include "scene.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    // Initialize game components
    initializeGame();
    setupView();
}

void MainWindow::initializeGame()
{
    // Initialize game scene
    gameScene = new QGraphicsScene(this);
    gameScene->setSceneRect(0, 0, 750, 750); // Large scene rect (750x750)
    gameScene->setBackgroundBrush(Qt::black); // Black background

    // Initialize game controller
    game = new Game(gameScene);
}

void MainWindow::setupView()
{
    // Initialize game view with correct dimensions
    gameView = new QGraphicsView(gameScene, this);
    gameView->setFixedSize(525, 450); // Window size from requirements: 525x450
    gameView->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    gameView->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    gameView->setRenderHint(QPainter::Antialiasing); // Smoother rendering
    gameView->setViewportUpdateMode(QGraphicsView::FullViewportUpdate);
    
    // Set up focus and event handling
    gameView->setFocusPolicy(Qt::StrongFocus);
    gameView->installEventFilter(this);
    gameView->setFocus();

    // Set the view as central widget
    setCentralWidget(gameView);
    
    // Adjust main window to fit the view
    adjustSize();
    setWindowTitle("Pokémon RPG");
    
    // Start the game to show title screen
    game->start();
}

MainWindow::~MainWindow()
{
    delete game;
    delete ui;
}

void MainWindow::keyPressEvent(QKeyEvent *event)
{
    if (!event->isAutoRepeat()) {
        game->handleKeyPress(event);
    }
}

void MainWindow::keyReleaseEvent(QKeyEvent *event)
{
    if (!event->isAutoRepeat()) {
        game->handleKeyRelease(event);
    }
}

bool MainWindow::eventFilter(QObject *obj, QEvent *event)
{
    if (obj == gameView) {
        if (event->type() == QEvent::KeyPress) {
            keyPressEvent(static_cast<QKeyEvent*>(event));
            return true;
        }
        else if (event->type() == QEvent::KeyRelease) {
            keyReleaseEvent(static_cast<QKeyEvent*>(event));
            return true;
        }
    }
    return QMainWindow::eventFilter(obj, event);
}

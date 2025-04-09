#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QKeyEvent>
#include <QGraphicsScene>
#include <QGraphicsView>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    // Initialize game scene and view
    gameScene = new QGraphicsScene(this);
    gameScene->setSceneRect(0, 0, 525, 450); // Set the scene size

    gameView = new QGraphicsView(gameScene, this);
    gameView->setFixedSize(525, 450);
    gameView->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    gameView->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    // Set the view as central widget
    setCentralWidget(gameView);

    // Initialize game controller
    game = new Game(gameScene);
    game->start();  // Start the game to show title screen

    // Set focus to receive keyboard events
    setFocusPolicy(Qt::StrongFocus);
}

MainWindow::~MainWindow()
{
    delete game;
    delete ui;
}

// Handle key press events
void MainWindow::keyPressEvent(QKeyEvent *event)
{
    // Pass key events to game controller
    game->handleKeyPress(event);
}

// Handle key release events
void MainWindow::keyReleaseEvent(QKeyEvent *event)
{
    // Pass key release events to game controller
    game->handleKeyRelease(event);
}

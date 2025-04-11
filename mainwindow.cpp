#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QKeyEvent>
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QPainter>
#include <QMenuBar>
#include <QMenu>
#include <QAction>
#include <QStatusBar>
#include <QMouseEvent>
#include "scene.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    // Initialize game components
    initializeGame();
    setupView();
    createActions();
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
    gameView->setMouseTracking(true); // Enable mouse tracking for coordinate display
    gameView->setFocus();

    // Set the view as central widget
    setCentralWidget(gameView);
    
    // Adjust main window to fit the view
    adjustSize();
    setWindowTitle("Pokémon RPG");
    
    // Start the game to show title screen
    game->start();
}

void MainWindow::createActions()
{
    // Create a debug mode toggle in the menu
    QMenu *debugMenu = menuBar()->addMenu("Debug");
    
    debugAction = new QAction("Toggle Debug Mode", this);
    debugAction->setShortcut(QKeySequence("Ctrl+D"));
    debugAction->setCheckable(true);
    debugAction->setChecked(false);
    
    connect(debugAction, &QAction::triggered, this, [this]() {
        Scene* currentScene = dynamic_cast<Scene*>(game->getCurrentScene());
        if (currentScene) {
            currentScene->toggleDebugMode();
            statusBar()->showMessage(QString("Debug mode %1").arg(
                currentScene->isDebugModeEnabled() ? "enabled" : "disabled"), 2000);
        }
    });
    
    debugMenu->addAction(debugAction);
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
        else if (event->type() == QEvent::MouseMove) {
            QMouseEvent *mouseEvent = static_cast<QMouseEvent*>(event);
            QPointF scenePos = gameView->mapToScene(mouseEvent->pos());
            
            // Pass to current scene for coordinate display
            Scene* currentScene = dynamic_cast<Scene*>(game->getCurrentScene());
            if (currentScene && currentScene->isDebugModeEnabled()) {
                currentScene->updateMousePosition(scenePos);
            }
        }
    }
    return QMainWindow::eventFilter(obj, event);
}

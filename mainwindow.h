#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QKeyEvent>
#include <QAction>
#include "game.h"

class QGraphicsScene;
class QGraphicsView;
class QKeyEvent;

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

protected:
    // Event handlers
    void keyPressEvent(QKeyEvent *event) override;
    void keyReleaseEvent(QKeyEvent *event) override;
    bool eventFilter(QObject *obj, QEvent *event) override;

private:
    Ui::MainWindow *ui;
    QGraphicsScene *gameScene;
    QGraphicsView *gameView;
    Game *game;
    QAction *debugAction; // Action to toggle debug mode

    void initializeGame();
    void setupView();
    void createActions();
};
#endif // MAINWINDOW_H

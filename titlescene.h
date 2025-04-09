#ifndef TITLESCENE_H
#define TITLESCENE_H

#include <QObject>
#include <QGraphicsScene>
#include <QGraphicsPixmapItem>
#include <QGraphicsTextItem>
#include <QTimer>

class Game;

class TitleScene : public QObject
{
    Q_OBJECT

public:
    explicit TitleScene(Game *game, QGraphicsScene *scene, QObject *parent = nullptr);
    ~TitleScene();

    void initialize();
    void cleanup();
    void update();
    void handleKeyPress(int key);

private slots:
    void animateTitle();

private:
    Game *game;
    QGraphicsScene *scene;
    QGraphicsPixmapItem *backgroundItem;
    QGraphicsPixmapItem *logoItem;
    QGraphicsTextItem *startTextItem;
    QTimer *animationTimer;

    int animationFrame;
    bool startTextVisible;

    void createBackground();
    void createLogo();
    void createStartText();
};

#endif // TITLESCENE_H

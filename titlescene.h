#ifndef TITLESCENE_H
#define TITLESCENE_H

#include "scene.h"
#include <QGraphicsPixmapItem>
#include <QGraphicsTextItem>
#include <QTimer>

class TitleScene : public Scene
{
    Q_OBJECT

public:
    explicit TitleScene(Game *game, QGraphicsScene *scene, QObject *parent = nullptr);
    ~TitleScene() override;

    void initialize() override;
    void cleanup() override;
    void handleKeyPress(int key) override;

private slots:
    void animateTitle();

private:
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

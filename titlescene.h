#ifndef TITLESCENE_H
#define TITLESCENE_H

#include "scene.h"
#include <QGraphicsPixmapItem>
#include <QGraphicsRectItem>
#include <QTimer>

class TitleScene : public Scene
{
    Q_OBJECT

public:
    explicit TitleScene(Game *game, QGraphicsScene *scene, QObject *parent = nullptr);
    ~TitleScene();

    void initialize() override;
    void cleanup() override;
    void handleKeyPress(int key) override;

signals:
    void startGame();

private:
    // Title scene constants - use window size for title scene
    static const int TITLE_WIDTH = 525;   // Initial title view width
    static const int TITLE_HEIGHT = 450;  // Initial title view height
    static const int VIEW_WIDTH = 525;    // Window width
    static const int VIEW_HEIGHT = 450;   // Window height

    QGraphicsPixmapItem* backgroundItem{nullptr};
    QGraphicsTextItem* titleTextItem{nullptr};
    QGraphicsTextItem* pressStartTextItem{nullptr};
    QGraphicsRectItem* textBackgroundItem{nullptr};
    QTimer* blinkTimer{nullptr};
    bool textVisible{true};
    QPointF cameraPos{0, 0};

    void createBackground();
    void createTitleText();
    void centerCamera();
    void blinkPressStartText();
};

#endif // TITLESCENE_H

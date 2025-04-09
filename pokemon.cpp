#include "pokemon.h"
#include <QDebug>

Pokemon::Pokemon(Type type) : type(type) {
    switch(type) {
        case CHARMANDER:
            name = "Charmander";
            imagePath = ":/Dataset/Image/battle/charmander.png";
            break;
        case SQUIRTLE:
            name = "Squirtle";
            imagePath = ":/Dataset/Image/battle/squirtle.png";
            break;
        case BULBASAUR:
            name = "Bulbasaur";
            imagePath = ":/Dataset/Image/battle/bulbasaur.png";
            break;
    }
    qDebug() << "Created Pokemon:" << name << "with image path:" << imagePath;
}

QPixmap Pokemon::getSprite() const {
    QPixmap sprite(imagePath);
    if (sprite.isNull()) {
        qDebug() << "Failed to load pokemon sprite:" << imagePath;
        QPixmap fallback(32, 32);
        fallback.fill(Qt::red);
        return fallback;
    }
    return sprite;
} 
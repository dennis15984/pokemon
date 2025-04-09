#ifndef POKEMON_H
#define POKEMON_H

#include <QString>
#include <QPixmap>

class Pokemon {
public:
    enum Type {
        CHARMANDER,
        SQUIRTLE,
        BULBASAUR
    };

    Pokemon(Type type);
    
    QString getName() const { return name; }
    QString getImagePath() const { return imagePath; }
    Type getType() const { return type; }
    QPixmap getSprite() const;

private:
    QString name;
    QString imagePath;
    Type type;
};

#endif // POKEMON_H 
#ifndef POKEMON_H
#define POKEMON_H

#include <QString>
#include <QPixmap>
#include <QVector>

// Forward declaration of Move class
class Move;

class Pokemon {
public:
    enum Type {
        CHARMANDER,
        SQUIRTLE,
        BULBASAUR
    };

    // Move class to store move information
    class Move {
    public:
        QString name;
        int power;
        int pp;
        // Add default constructor
        Move() : name(""), power(0), pp(0) {}
        Move(const QString& n, int p, int ppVal) : name(n), power(p), pp(ppVal) {}
    };

    Pokemon(Type type);
    
    // Getters
    QString getName() const { return name; }
    QString getImagePath() const { return imagePath; }
    Type getType() const { return type; }
    QPixmap getSprite() const;
    int getLevel() const { return level; }
    int getAttack() const { return attack; }
    int getDefense() const { return defense; }
    int getMaxHp() const { return maxHp; }
    int getCurrentHp() const { return currentHp; }
    const QVector<Move>& getMoves() const { return moves; }

    // Setters
    void setLevel(int newLevel) { level = newLevel; }
    void setCurrentHp(int hp) { currentHp = hp; }
    void addMove(const QString& name, int power, int pp) { moves.append(Move(name, power, pp)); }

private:
    QString name;
    QString imagePath;
    Type type;
    int level{1};  // All Pokemon start at level 1
    int attack{5};  // Base attack for all starters
    int defense{5}; // Base defense for all starters
    int maxHp{30};  // Base max HP for all starters
    int currentHp{30}; // Current HP starts at max
    QVector<Move> moves; // Vector to store moves
};

#endif // POKEMON_H 
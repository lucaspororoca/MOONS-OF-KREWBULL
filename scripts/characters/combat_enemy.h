#ifndef COMBAT_ENEMY_H
#define COMBAT_ENEMY_H

#include <SDL2/SDL.h>
#include <string>

class CavesOfChaos;
class Player;

class CombatEnemy {
public:
    struct BattleTurnResult {
        std::string moveName;
        bool hit = false;
        int roll = 0;
        int amount = 0;
        int healingDie = 0;
        int damageDie = 0;
        int discardedDamageDie = 0;
        int strengthModifier = 0;
        int constitutionModifier = 0;
        int damageBonus = 0;
        int hpLost = 0;
        bool healing = false;
    };

    virtual ~CombatEnemy() = default;

    virtual void update(float dt, const CavesOfChaos& map,
                        const Player& player) = 0;
    virtual void render(SDL_Renderer* renderer, int camX, int camY) = 0;
    virtual float getX() const = 0;
    virtual float getY() const = 0;
    virtual int getHP() const = 0;
    virtual int getMaxHP() const = 0;
    virtual int getArmorClass() const = 0;
    virtual int getDexterity() const = 0;
    virtual int getExperienceReward() const = 0;
    virtual bool isAlive() const = 0;
    virtual void getHitbox(float& hx, float& hy, int& hw, int& hh) const = 0;
    virtual void takeDamage(int damage) = 0;
    virtual void beginBattleTurn() = 0;
    virtual void endBattleTurn() = 0;
    virtual bool hasAvailableBattleMove() const = 0;
    virtual BattleTurnResult takeBattleTurn(Player& player) = 0;
    virtual const char* battleName() const = 0;
    virtual const char* battleSpritePath() const = 0;
    virtual bool battleSpriteFillsScreen() const = 0;
};

#endif

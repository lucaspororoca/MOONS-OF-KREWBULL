#ifndef RAT_H
#define RAT_H

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <string>
#include "scripts/characters/combat_enemy.h"

class Rat : public CombatEnemy {
public:
    static const int SPRITE_SIZE = 185;
    static const int MAX_STAMINA = 80;
    static const int ARMOR_CLASS = 8;

    Rat(SDL_Renderer* renderer, float startX, float startY);
    ~Rat() override;

    void update(float dt, const CavesOfChaos& map,
                const Player& player) override;
    void render(SDL_Renderer* renderer, int camX, int camY) override;

    float getX() const override { return x; }
    float getY() const override { return y; }
    int getHP() const override { return hp; }
    int getMaxHP() const override { return maxHp; }
    int getArmorClass() const override { return ARMOR_CLASS; }
    int getExperienceReward() const override { return 25; }
    int getStrength() const { return strength; }
    int getDexterity() const override { return dexterity; }
    int getConstitution() const { return constitution; }
    int getIntellect() const { return intellect; }
    int getWisdom() const { return wisdom; }
    int getCharisma() const { return charisma; }
    int getStamina() const { return static_cast<int>(stamina); }
    bool isAlive() const override { return hp > 0; }

    static int abilityModifier(int score);
    void getHitbox(float& hx, float& hy, int& hw, int& hh) const override;
    void takeDamage(int damage) override;
    void beginBattleTurn() override {}
    void endBattleTurn() override;
    bool hasAvailableBattleMove() const override;
    BattleTurnResult takeBattleTurn(Player& player) override;
    const char* battleName() const override { return "Rat"; }
    const char* battleSpritePath() const override {
        return "resources/sprites/battle/characters/enemy/rat/rat_battle.png";
    }
    bool battleSpriteFillsScreen() const override { return false; }

private:
    enum Direction { DIR_LEFT, DIR_RIGHT };

    bool loadTexture(SDL_Renderer* renderer, const std::string& path,
                     SDL_Texture*& texture);
    float distanceTo(float px, float py) const;
    void pickNewWanderDirection();
    bool tryMove(float moveDx, float moveDy, float speed, float dt,
                 const CavesOfChaos& map, const Player& player);

    float x;
    float y;
    float wanderSpeed;
    float pursueSpeed;
    int hp;
    int maxHp;
    int strength;
    int dexterity;
    int constitution;
    int intellect;
    int wisdom;
    int charisma;
    int biteCooldown;
    bool biteCooldownStartedThisTurn;
    float stamina;
    float visionRadius;
    bool exhausted;
    Direction currentDirection;
    bool moving;
    float wanderTimer;
    float wanderDx;
    float wanderDy;
    SDL_Texture* leftTexture;
    SDL_Texture* rightTexture;
};

#endif

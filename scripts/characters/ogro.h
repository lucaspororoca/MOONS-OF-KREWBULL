#ifndef OGRO_H
#define OGRO_H

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <string>
#include "scripts/characters/combat_enemy.h"

class Player;
class CavesOfChaos;

class Ogro : public CombatEnemy {
public:
    static const int SPRITE_SIZE = 350;
    static const int MAX_STAMINA = 120;
    static const int ARMOR_CLASS = 11;

    using BattleTurnResult = CombatEnemy::BattleTurnResult;

    Ogro(SDL_Renderer* renderer, float startX, float startY);
    ~Ogro();

    void update(float dt, const CavesOfChaos& map,
                const Player& player) override;
    void render(SDL_Renderer* renderer, int camX, int camY) override;

    float getX() const override { return x; }
    float getY() const override { return y; }
    int   getHP() const override { return hp; }
    int   getMaxHP() const override { return maxHp; }
    int   getArmorClass() const override { return ARMOR_CLASS; }
    int   getExperienceReward() const override { return 200; }
    int   getStrength() const { return strength; }
    int   getDexterity() const { return dexterity; }
    int   getConstitution() const { return constitution; }
    int   getIntellect() const { return intellect; }
    int   getWisdom() const { return wisdom; }
    int   getCharisma() const { return charisma; }
    static int abilityModifier(int score);
    int   getStamina() const { return static_cast<int>(stamina); }
    bool  isAlive() const override { return hp > 0; }

    void getHitbox(float& hx, float& hy, int& hw, int& hh) const override;
    void takeDamage(int dmg) override;
    void restoreHP(int amount);
    void beginBattleTurn() override;
    void endBattleTurn() override {}
    bool hasAvailableBattleMove() const override;
    BattleTurnResult takeBattleTurn(Player& player) override;
    const char* battleName() const override { return "Ogro"; }
    const char* battleSpritePath() const override {
        return "resources/sprites/battle/characters/enemy/ogro/ogro_battle.png";
    }
    bool battleSpriteFillsScreen() const override { return true; }

private:
    enum Direction { DIR_UP, DIR_DOWN, DIR_LEFT, DIR_RIGHT };
    enum State { STATE_IDLE, STATE_WANDER, STATE_PURSUE, STATE_EXHAUSTED };

    static const int WALK_FRAMES = 3;
    static const float FRAME_TIME;

    bool loadTexture(SDL_Renderer* renderer,
                     const std::string& path,
                     SDL_Texture*& texture);
    bool loadHudTexture(SDL_Renderer* renderer,
                        const std::string& path);

    float distanceTo(float px, float py) const;
    void  pickNewWanderDirection();
    bool  tryMove(float moveDx, float moveDy, float speed, float dt,
                  const CavesOfChaos& map, const Player& player);

    float x, y;
    float wanderSpeed;
    float pursueSpeed;

    int   hp;
    int   maxHp;
    int   strength;
    int   dexterity;
    int   constitution;
    int   intellect;
    int   wisdom;
    int   charisma;
    int   boneInCutCooldown;
    int   fatigueCooldown;
    float stamina;

    float visionRadius;

    Direction currentDirection;
    State     state;

    bool  moving;
    float animTime;
    int   animFrame;

    float wanderTimer;
    float wanderDx;
    float wanderDy;

    SDL_Texture* texUp;
    SDL_Texture* texDown;
    SDL_Texture* texLeft;
    SDL_Texture* texRight;

    SDL_Texture* texWalkUp[WALK_FRAMES];
    SDL_Texture* texWalkDown[WALK_FRAMES];
    SDL_Texture* texWalkLeft[WALK_FRAMES];
    SDL_Texture* texWalkRight[WALK_FRAMES];

    SDL_Texture* hudTex;
    int hudW;
    int hudH;
};

#endif
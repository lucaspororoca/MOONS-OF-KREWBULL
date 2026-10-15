#ifndef PLAYER_H
#define PLAYER_H

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_ttf.h>
#include <string>
#include <vector>
#include "scripts/resources/player_save.h"

class CavesOfChaos;

class Player {
public:
    static const int SPRITE_SIZE = 125;
    static const int MAX_HP = 100;
    static constexpr int MAX_LEVEL = 20;

    std::string name;
    std::string characterClass;
    std::string subclass;
    std::string race;
    std::string subrace;
    std::string gender;
    std::string spritePath;
    std::vector<player_save::Skill> skills;
    std::vector<player_save::BagItem> inventory;
    int level;
    int experience;

    int strength;
    int defense;
    int constitution;
    int dexterity;
    int charisma;
    int intellect;
    int wisdom;

    int hp;
    int maxHp;
    int armorClass;

    Player(SDL_Renderer* renderer, float startX, float startY);
    ~Player();

    void handleEvent(const SDL_Event& event);
    void update(float dt, const CavesOfChaos& map);
    void initializeHitPoints();
    bool gainLevel();
    int experienceToNextLevel() const;
    int gainExperience(int amount);
    void clearMovementInput();
    void render(SDL_Renderer* renderer, int camX, int camY);
    void renderNameplate(SDL_Renderer* renderer, int camX, int camY) const;
    bool setSpriteDirectory(SDL_Renderer* renderer,
                            const std::string& directory);

    float getX() const { return x; }
    float getY() const { return y; }
    void setPosition(float newX, float newY) { x = newX; y = newY; }

    void getHitbox(float& hx, float& hy, int& hw, int& hh) const;

private:
    enum Direction { DIR_UP, DIR_DOWN, DIR_LEFT, DIR_RIGHT };

    static const int WALK_FRAMES = 3;
    static const float FRAME_TIME;

    bool loadTexture(SDL_Renderer* renderer,
                     const std::string& path,
                     SDL_Texture*& texture);
    void destroySpriteTextures();
    std::vector<SDL_Texture**> spriteTextureDestinations();

    float x, y;
    float speed;
    int   width, height;

    Direction currentDirection;
    bool  moving;
    float animTime;
    int   animFrame;

    SDL_Texture* texMoveUp;
    SDL_Texture* texMoveDown;
    SDL_Texture* texMoveLeft;
    SDL_Texture* texMoveRight;

    SDL_Texture* texWalkUp[WALK_FRAMES];
    SDL_Texture* texWalkDown[WALK_FRAMES];
    SDL_Texture* texWalkLeft[WALK_FRAMES];
    SDL_Texture* texWalkRight[WALK_FRAMES];
    SDL_Texture* indicatorTexture;
    TTF_Font* nameFont;

    bool keys[SDL_NUM_SCANCODES];
};

#endif
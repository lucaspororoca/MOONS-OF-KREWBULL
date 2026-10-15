#include "scripts/characters/ogro.h"
#include "scripts/resources/resource_archive.h"
#include "scripts/characters/player.h"
#include "scripts/maps/caves_of_chaos.h"
#include "scripts/dice/dice_1d8.h"
#include "scripts/dice/dice_1d10.h"
#include "scripts/dice/dice_1d4.h"
#include "scripts/dice/dice_1d20.h"
#include <cmath>
#include <cstdlib>
#include <ctime>
#include <iostream>
#include <algorithm>

static bool ogroRngSeeded = false;

const float Ogro::FRAME_TIME = 0.15f;

static bool rectsOverlap(float ax, float ay, int aw, int ah,
                         float bx, float by, int bw, int bh)
{
    return ax < bx + bw && ax + aw > bx &&
           ay < by + bh && ay + ah > by;
}

Ogro::Ogro(SDL_Renderer* renderer, float startX, float startY)
    : x(startX), y(startY),
      wanderSpeed(50.0f),
      pursueSpeed(125.0f),
      hp(0),
      maxHp(0),
      strength(19),
      dexterity(8),
      constitution(16),
      intellect(5),
      wisdom(7),
      charisma(7),
      boneInCutCooldown(0),
      fatigueCooldown(0),
      stamina(static_cast<float>(MAX_STAMINA)),
      visionRadius(250.0f),
      currentDirection(DIR_DOWN),
      state(STATE_WANDER),
      moving(false),
      animTime(0.0f),
      animFrame(0),
      wanderTimer(0.0f),
      wanderDx(0.0f),
      wanderDy(0.0f),
      texUp(nullptr), texDown(nullptr),
      texLeft(nullptr), texRight(nullptr),
      hudTex(nullptr), hudW(0), hudH(0)
{
    maxHp = 21;
    for (int die = 0; die < 7; ++die) {
        maxHp += Dice1d10::roll();
    }
    hp = maxHp;

    if (!ogroRngSeeded) {
        std::srand(static_cast<unsigned>(std::time(nullptr)));
        ogroRngSeeded = true;
    }

    for (int i = 0; i < WALK_FRAMES; ++i) {
        texWalkUp[i]    = nullptr;
        texWalkDown[i]  = nullptr;
        texWalkLeft[i]  = nullptr;
        texWalkRight[i] = nullptr;
    }

    loadTexture(renderer, "resources/sprites/characters/ogro_01/stop_up.png",    texUp);
    loadTexture(renderer, "resources/sprites/characters/ogro_01/stop_down.png",  texDown);
    loadTexture(renderer, "resources/sprites/characters/ogro_01/stop_left.png",  texLeft);
    loadTexture(renderer, "resources/sprites/characters/ogro_01/stop_right.png", texRight);

    loadTexture(renderer, "resources/sprites/characters/ogro_01/walk_up_01.png", texWalkUp[0]);
    loadTexture(renderer, "resources/sprites/characters/ogro_01/stop_up.png",    texWalkUp[1]);
    loadTexture(renderer, "resources/sprites/characters/ogro_01/walk_up_02.png", texWalkUp[2]);

    loadTexture(renderer, "resources/sprites/characters/ogro_01/walk_down_01.png", texWalkDown[0]);
    loadTexture(renderer, "resources/sprites/characters/ogro_01/stop_down.png",    texWalkDown[1]);
    loadTexture(renderer, "resources/sprites/characters/ogro_01/walk_down_02.png", texWalkDown[2]);

    loadTexture(renderer, "resources/sprites/characters/ogro_01/walk_left_01.png", texWalkLeft[0]);
    loadTexture(renderer, "resources/sprites/characters/ogro_01/stop_left.png",    texWalkLeft[1]);
    loadTexture(renderer, "resources/sprites/characters/ogro_01/walk_left_02.png", texWalkLeft[2]);

    loadTexture(renderer, "resources/sprites/characters/ogro_01/walk_right_01.png", texWalkRight[0]);
    loadTexture(renderer, "resources/sprites/characters/ogro_01/stop_right.png",    texWalkRight[1]);
    loadTexture(renderer, "resources/sprites/characters/ogro_01/walk_right_02.png", texWalkRight[2]);

    loadHudTexture(renderer, "resources/interfaces/indicators/indicator_ogro.png");

    pickNewWanderDirection();
}

int Ogro::abilityModifier(int score) {
    return static_cast<int>(std::floor((score - 10) / 2.0));
}

void Ogro::beginBattleTurn() {
    if (boneInCutCooldown > 0) --boneInCutCooldown;
    if (fatigueCooldown > 0) --fatigueCooldown;
}

bool Ogro::hasAvailableBattleMove() const {
    return isAlive() &&
        (boneInCutCooldown == 0 ||
         (fatigueCooldown == 0 && hp < maxHp));
}

Ogro::BattleTurnResult Ogro::takeBattleTurn(Player& player) {
    BattleTurnResult result;
    if (boneInCutCooldown > 0 &&
        fatigueCooldown == 0 && hp < maxHp) {
        result.moveName = "Fatigue";
        result.healingDie = Dice1d4::roll();
        result.constitutionModifier = abilityModifier(constitution);
        result.amount = std::max(
            0, result.healingDie + result.constitutionModifier);
        const int healed = std::min(result.amount, maxHp - hp);
        result.amount = healed;
        hp += healed;
        result.moveName = "Fatigue";
        result.healing = true;
        fatigueCooldown = 8;
        return result;
    }

    if (boneInCutCooldown > 0) return result;

    result.moveName = "Bone-in Cut";
    result.roll = Dice1d20::roll() + abilityModifier(strength);
    if (result.roll >= player.armorClass) {
        result.hit = true;
        const int firstDie = Dice1d8::roll();
        const int secondDie = Dice1d8::roll();
        result.damageDie = std::max(firstDie, secondDie);
        result.discardedDamageDie = std::min(firstDie, secondDie);
        result.strengthModifier = 4;
        result.amount = result.damageDie + 4;
        const int previousHp = player.hp;
        player.hp = std::max(0, player.hp - result.amount);
        result.hpLost = previousHp - player.hp;
    }
    boneInCutCooldown = 10;
    return result;
}

Ogro::~Ogro() {
    if (texUp)    SDL_DestroyTexture(texUp);
    if (texDown)  SDL_DestroyTexture(texDown);
    if (texLeft)  SDL_DestroyTexture(texLeft);
    if (texRight) SDL_DestroyTexture(texRight);
    if (hudTex)   SDL_DestroyTexture(hudTex);

    for (int i = 0; i < WALK_FRAMES; ++i) {
        if (texWalkUp[i])    SDL_DestroyTexture(texWalkUp[i]);
        if (texWalkDown[i])  SDL_DestroyTexture(texWalkDown[i]);
        if (texWalkLeft[i])  SDL_DestroyTexture(texWalkLeft[i]);
        if (texWalkRight[i]) SDL_DestroyTexture(texWalkRight[i]);
    }
}

bool Ogro::loadTexture(SDL_Renderer* renderer,
                       const std::string& path,
                       SDL_Texture*& texture)
{
    SDL_Surface* surface = resource_archive::loadResourceImage(path);
    if (!surface) {
        std::cerr << "Falha ao carregar " << path
                  << ": " << IMG_GetError() << std::endl;
        return false;
    }

    SDL_Surface* scaled = SDL_CreateRGBSurfaceWithFormat(
        0, SPRITE_SIZE, SPRITE_SIZE, 32, SDL_PIXELFORMAT_RGBA32);
    if (!scaled) {
        std::cerr << "Falha ao criar surface escalada: "
                  << SDL_GetError() << std::endl;
        SDL_FreeSurface(surface);
        return false;
    }

    SDL_SetSurfaceBlendMode(surface, SDL_BLENDMODE_BLEND);
    SDL_BlitScaled(surface, nullptr, scaled, nullptr);
    SDL_SetSurfaceBlendMode(scaled, SDL_BLENDMODE_BLEND);

    texture = SDL_CreateTextureFromSurface(renderer, scaled);
    SDL_FreeSurface(scaled);
    SDL_FreeSurface(surface);

    if (!texture) {
        std::cerr << "Falha ao criar textura de " << path
                  << ": " << SDL_GetError() << std::endl;
        return false;
    }

    SDL_SetTextureScaleMode(texture, SDL_ScaleModeNearest);
    return true;
}

bool Ogro::loadHudTexture(SDL_Renderer* renderer,
                          const std::string& path)
{
    SDL_Surface* surface = resource_archive::loadResourceImage(path);
    if (!surface) {
        std::cerr << "Falha ao carregar " << path
                  << ": " << IMG_GetError() << std::endl;
        return false;
    }

    hudTex = SDL_CreateTextureFromSurface(renderer, surface);
    if (!hudTex) {
        std::cerr << "Falha ao criar textura de " << path
                  << ": " << SDL_GetError() << std::endl;
        SDL_FreeSurface(surface);
        return false;
    }

    hudW = surface->w;
    hudH = surface->h;
    SDL_FreeSurface(surface);

    SDL_SetTextureScaleMode(hudTex, SDL_ScaleModeNearest);
    return true;
}

void Ogro::getHitbox(float& hx, float& hy, int& hw, int& hh) const {
    hw = SPRITE_SIZE * 6 / 10;
    hh = SPRITE_SIZE * 6 / 10;
    hx = x - hw / 2.0f;
    hy = y + SPRITE_SIZE / 2.0f - hh;
}

void Ogro::takeDamage(int dmg) {
    hp = std::max(0, hp - std::max(0, dmg));
}

void Ogro::restoreHP(int amount) {
    hp = std::min(maxHp, hp + std::max(0, amount));
}

float Ogro::distanceTo(float px, float py) const {
    float dx = px - x;
    float dy = py - y;
    return std::sqrt(dx * dx + dy * dy);
}

void Ogro::pickNewWanderDirection() {
    int dir = std::rand() % 4;
    switch (dir) {
        case 0: wanderDx =  0.0f; wanderDy = -1.0f; currentDirection = DIR_UP;    break;
        case 1: wanderDx =  0.0f; wanderDy =  1.0f; currentDirection = DIR_DOWN;  break;
        case 2: wanderDx = -1.0f; wanderDy =  0.0f; currentDirection = DIR_LEFT;  break;
        default: wanderDx = 1.0f; wanderDy =  0.0f; currentDirection = DIR_RIGHT; break;
    }
    wanderTimer = 1.5f + (std::rand() % 150) / 100.0f;
}

bool Ogro::tryMove(float moveDx, float moveDy, float speed, float dt,
                   const CavesOfChaos& map, const Player& player)
{
    bool moved = false;

    float phx, phy; int phw, phh;
    player.getHitbox(phx, phy, phw, phh);

    float chx, chy; int chw, chh;
    getHitbox(chx, chy, chw, chh);
    bool alreadyOverlapping = rectsOverlap(chx, chy, chw, chh,
                                           phx, phy, phw, phh);

    if (moveDx != 0.0f) {
        float newX = x + moveDx * speed * dt;
        float hx, hy; int hw, hh;
        getHitbox(hx, hy, hw, hh);
        float testX = hx + (newX - x);
        bool blockedByPlayer = !alreadyOverlapping &&
                               rectsOverlap(testX, hy, hw, hh,
                                            phx, phy, phw, phh);
        if (!map.collides(testX, hy, hw, hh) && !blockedByPlayer) {
            x = newX;
            moved = true;
        }
    }

    if (moveDy != 0.0f) {
        float newY = y + moveDy * speed * dt;
        float hx, hy; int hw, hh;
        getHitbox(hx, hy, hw, hh);
        float testY = hy + (newY - y);
        bool blockedByPlayer = !alreadyOverlapping &&
                               rectsOverlap(hx, testY, hw, hh,
                                            phx, phy, phw, phh);
        if (!map.collides(hx, testY, hw, hh) && !blockedByPlayer) {
            y = newY;
            moved = true;
        }
    }

    return moved;
}

void Ogro::update(float dt, const CavesOfChaos& map, const Player& player) {
    if (hp <= 0) return;

    float dist = distanceTo(player.getX(), player.getY());
    bool canSeePlayer = dist <= visionRadius;

    if (state == STATE_EXHAUSTED) {
        stamina += 30.0f * dt;
        if (stamina > MAX_STAMINA) stamina = MAX_STAMINA;

        moving = false;
        animTime = 0.0f;
        animFrame = 0;

        if (stamina >= MAX_STAMINA * 0.6f && canSeePlayer) {
            state = STATE_PURSUE;
        } else if (stamina >= MAX_STAMINA) {
            state = STATE_WANDER;
            pickNewWanderDirection();
        }
        return;
    }

    if (stamina <= 0.0f) {
        stamina = 0.0f;
        state = STATE_EXHAUSTED;
        return;
    }

    float moveDx = 0.0f;
    float moveDy = 0.0f;
    float curSpeed = 0.0f;

    if (canSeePlayer) {
        state = STATE_PURSUE;

        float pdx = player.getX() - x;
        float pdy = player.getY() - y;
        float len = std::sqrt(pdx * pdx + pdy * pdy);
        if (len > 0.001f) {
            moveDx = pdx / len;
            moveDy = pdy / len;
        }
        curSpeed = pursueSpeed;
        stamina -= 15.0f * dt;
    } else {
        state = STATE_WANDER;

        wanderTimer -= dt;
        if (wanderTimer <= 0.0f) {
            pickNewWanderDirection();
        }

        moveDx = wanderDx;
        moveDy = wanderDy;
        curSpeed = wanderSpeed;
        stamina -= 3.0f * dt;
    }

    if (stamina < 0.0f) stamina = 0.0f;

    if (moveDx != 0.0f || moveDy != 0.0f) {
        if (std::abs(moveDx) > std::abs(moveDy)) {
            currentDirection = moveDx > 0.0f ? DIR_RIGHT : DIR_LEFT;
        } else if (moveDy != 0.0f) {
            currentDirection = moveDy > 0.0f ? DIR_DOWN : DIR_UP;
        }
    }

    bool didMove = tryMove(moveDx, moveDy, curSpeed, dt, map, player);

    if (!didMove && state == STATE_WANDER) {
        pickNewWanderDirection();
    }

    moving = didMove;
    if (moving) {
        animTime += dt;
        while (animTime >= FRAME_TIME) {
            animTime -= FRAME_TIME;
            animFrame = (animFrame + 1) % WALK_FRAMES;
        }
    } else {
        animTime = 0.0f;
        animFrame = 0;
    }
}

void Ogro::render(SDL_Renderer* renderer, int camX, int camY) {
    SDL_Texture* tex = nullptr;

    if (moving) {
        switch (currentDirection) {
            case DIR_UP:    tex = texWalkUp[animFrame];    break;
            case DIR_DOWN:  tex = texWalkDown[animFrame];  break;
            case DIR_LEFT:  tex = texWalkLeft[animFrame];  break;
            case DIR_RIGHT: tex = texWalkRight[animFrame]; break;
        }
    } else {
        switch (currentDirection) {
            case DIR_UP:    tex = texUp;    break;
            case DIR_DOWN:  tex = texDown;  break;
            case DIR_LEFT:  tex = texLeft;  break;
            case DIR_RIGHT: tex = texRight; break;
        }
    }

    if (!tex) return;

    SDL_Rect dst;
    dst.w = SPRITE_SIZE;
    dst.h = SPRITE_SIZE;
    dst.x = static_cast<int>(x) - SPRITE_SIZE / 2 - camX;
    dst.y = static_cast<int>(y) - SPRITE_SIZE / 2 - camY;

    SDL_RenderCopy(renderer, tex, nullptr, &dst);

    if (hudTex && hudW > 0 && hudH > 0) {
        SDL_Rect hudDst;
        hudDst.w = hudW;
        hudDst.h = hudH;
        hudDst.x = static_cast<int>(x) - hudW / 2 - camX;
        hudDst.y = dst.y - hudH - 8;
        SDL_RenderCopy(renderer, hudTex, nullptr, &hudDst);
    }
}
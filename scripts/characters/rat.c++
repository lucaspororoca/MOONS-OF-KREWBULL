#include "scripts/characters/rat.h"
#include "scripts/characters/player.h"
#include "scripts/dice/dice_1d4.h"
#include "scripts/dice/dice_1d20.h"
#include "scripts/maps/caves_of_chaos.h"
#include "scripts/resources/resource_archive.h"
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <stdexcept>

namespace {

bool overlaps(float ax, float ay, int aw, int ah,
              float bx, float by, int bw, int bh) {
    return ax < bx + bw && ax + aw > bx &&
           ay < by + bh && ay + ah > by;
}

}

Rat::Rat(SDL_Renderer* renderer, float startX, float startY)
    : x(startX),
      y(startY),
      wanderSpeed(75.0f),
      pursueSpeed(185.0f),
      hp(1),
      maxHp(1),
      strength(2),
      dexterity(11),
      constitution(9),
      intellect(2),
      wisdom(10),
      charisma(4),
      biteCooldown(0),
      biteCooldownStartedThisTurn(false),
      stamina(static_cast<float>(MAX_STAMINA)),
      visionRadius(600.0f),
      exhausted(false),
      currentDirection(DIR_RIGHT),
      moving(false),
      wanderTimer(0.0f),
      wanderDx(0.0f),
      wanderDy(0.0f),
      leftTexture(nullptr),
      rightTexture(nullptr) {
    maxHp = 1 + (Dice1d4::roll() - 1);
    hp = maxHp;

    if (!loadTexture(
            renderer, "resources/sprites/characters/rat/stop_left.png",
            leftTexture) ||
        !loadTexture(
            renderer, "resources/sprites/characters/rat/stop_right.png",
            rightTexture)) {
        if (leftTexture) SDL_DestroyTexture(leftTexture);
        if (rightTexture) SDL_DestroyTexture(rightTexture);
        leftTexture = nullptr;
        rightTexture = nullptr;
        throw std::runtime_error("Falha ao carregar sprites do rat.");
    }

    pickNewWanderDirection();
}

Rat::~Rat() {
    if (leftTexture) SDL_DestroyTexture(leftTexture);
    if (rightTexture) SDL_DestroyTexture(rightTexture);
}

bool Rat::loadTexture(SDL_Renderer* renderer, const std::string& path,
                      SDL_Texture*& texture) {
    SDL_Surface* surface = resource_archive::loadResourceImage(path);
    if (!surface) {
        std::cerr << "Falha ao carregar " << path
                  << ": " << IMG_GetError() << std::endl;
        return false;
    }
    texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_FreeSurface(surface);
    if (!texture) {
        std::cerr << "Falha ao criar textura de " << path
                  << ": " << SDL_GetError() << std::endl;
        return false;
    }
    SDL_SetTextureScaleMode(texture, SDL_ScaleModeNearest);
    return true;
}

int Rat::abilityModifier(int score) {
    return static_cast<int>(std::floor((score - 10) / 2.0));
}

void Rat::getHitbox(float& hx, float& hy, int& hw, int& hh) const {
    hw = SPRITE_SIZE * 7 / 10;
    hh = SPRITE_SIZE * 6 / 10;
    hx = x - hw / 2.0f;
    hy = y + SPRITE_SIZE / 2.0f - hh;
}

void Rat::takeDamage(int damage) {
    hp = std::max(0, hp - std::max(0, damage));
}

void Rat::endBattleTurn() {
    if (biteCooldownStartedThisTurn) {
        biteCooldownStartedThisTurn = false;
    } else if (biteCooldown > 0) {
        --biteCooldown;
    }
}

bool Rat::hasAvailableBattleMove() const {
    return isAlive() && biteCooldown == 0;
}

CombatEnemy::BattleTurnResult Rat::takeBattleTurn(Player& player) {
    BattleTurnResult result;
    if (!hasAvailableBattleMove()) return result;

    result.moveName = "Bite";
    result.roll = Dice1d20::roll() + abilityModifier(dexterity);
    if (result.roll >= player.armorClass) {
        result.hit = true;
        result.damageDie = Dice1d4::roll();
        result.damageBonus = 1;
        result.amount = 1 + result.damageDie;
        const int oldHp = player.hp;
        player.hp = std::max(0, player.hp - result.amount);
        result.hpLost = oldHp - player.hp;
    }
    biteCooldown = 3;
    biteCooldownStartedThisTurn = true;
    return result;
}

float Rat::distanceTo(float px, float py) const {
    const float dx = px - x;
    const float dy = py - y;
    return std::sqrt(dx * dx + dy * dy);
}

void Rat::pickNewWanderDirection() {
    const int direction = std::rand() % 4;
    wanderDx = direction < 2 ? (direction == 0 ? -1.0f : 1.0f) : 0.0f;
    wanderDy = direction >= 2 ? (direction == 2 ? -1.0f : 1.0f) : 0.0f;
    if (wanderDx != 0.0f) {
        currentDirection = wanderDx < 0.0f ? DIR_LEFT : DIR_RIGHT;
    }
    wanderTimer = 1.5f + (std::rand() % 150) / 100.0f;
}

bool Rat::tryMove(float moveDx, float moveDy, float speed, float dt,
                  const CavesOfChaos& map, const Player& player) {
    bool moved = false;
    float playerX, playerY;
    int playerW, playerH;
    player.getHitbox(playerX, playerY, playerW, playerH);

    float hitX, hitY;
    int hitW, hitH;
    getHitbox(hitX, hitY, hitW, hitH);
    const bool alreadyOverlapping =
        overlaps(hitX, hitY, hitW, hitH,
                 playerX, playerY, playerW, playerH);

    if (moveDx != 0.0f) {
        const float newX = x + moveDx * speed * dt;
        const float testX = hitX + newX - x;
        const bool blockedByPlayer = !alreadyOverlapping &&
            overlaps(testX, hitY, hitW, hitH,
                     playerX, playerY, playerW, playerH);
        if (!map.collides(testX, hitY, hitW, hitH) && !blockedByPlayer) {
            x = newX;
            moved = true;
        }
    }

    if (moveDy != 0.0f) {
        const float newY = y + moveDy * speed * dt;
        const float testY = hitY + newY - y;
        const bool blockedByPlayer = !alreadyOverlapping &&
            overlaps(hitX, testY, hitW, hitH,
                     playerX, playerY, playerW, playerH);
        if (!map.collides(hitX, testY, hitW, hitH) && !blockedByPlayer) {
            y = newY;
            moved = true;
        }
    }
    return moved;
}

void Rat::update(float dt, const CavesOfChaos& map, const Player& player) {
    if (!isAlive()) return;
    const bool seesPlayer =
        distanceTo(player.getX(), player.getY()) <= visionRadius;
    if (exhausted) {
        stamina = std::min(
            static_cast<float>(MAX_STAMINA), stamina + 30.0f * dt);
        moving = false;
        if (stamina >= MAX_STAMINA * 0.6f && seesPlayer) {
            exhausted = false;
        } else if (stamina >= MAX_STAMINA) {
            exhausted = false;
            pickNewWanderDirection();
        }
        if (exhausted) return;
    }
    if (stamina <= 0.0f) {
        stamina = 0.0f;
        exhausted = true;
        moving = false;
        return;
    }

    float moveX = 0.0f;
    float moveY = 0.0f;
    float speed = 0.0f;

    if (seesPlayer) {
        const float dx = player.getX() - x;
        const float dy = player.getY() - y;
        const float length = std::sqrt(dx * dx + dy * dy);
        if (length > 0.001f) {
            moveX = dx / length;
            moveY = dy / length;
        }
        speed = pursueSpeed;
        stamina = std::max(0.0f, stamina - 15.0f * dt);
    } else {
        wanderTimer -= dt;
        if (wanderTimer <= 0.0f) pickNewWanderDirection();
        moveX = wanderDx;
        moveY = wanderDy;
        speed = wanderSpeed;
        stamina = std::max(0.0f, stamina - 3.0f * dt);
    }

    if (moveX < 0.0f) currentDirection = DIR_LEFT;
    else if (moveX > 0.0f) currentDirection = DIR_RIGHT;

    moving = tryMove(moveX, moveY, speed, dt, map, player);
    if (!moving && !seesPlayer) pickNewWanderDirection();
}

void Rat::render(SDL_Renderer* renderer, int camX, int camY) {
    SDL_Texture* texture =
        currentDirection == DIR_LEFT ? leftTexture : rightTexture;
    if (!texture) return;
    const SDL_Rect destination = {
        static_cast<int>(x) - SPRITE_SIZE / 2 - camX,
        static_cast<int>(y) - SPRITE_SIZE / 2 - camY,
        SPRITE_SIZE, SPRITE_SIZE};
    SDL_RenderCopy(renderer, texture, nullptr, &destination);
}
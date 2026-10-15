#include "scripts/characters/player.h"
#include "scripts/resources/resource_archive.h"
#include "scripts/maps/caves_of_chaos.h"
#include "scripts/dice/dice_1d10.h"
#include <algorithm>
#include <iostream>
#include <cmath>

const float Player::FRAME_TIME = 0.15f;

Player::Player(SDL_Renderer* renderer, float startX, float startY)
    : level(1),
      experience(0),
      strength(0), defense(0), constitution(0),
      dexterity(0), charisma(0), intellect(0), wisdom(0),
      hp(MAX_HP), maxHp(MAX_HP), armorClass(10),
      x(startX), y(startY),
      speed(200.0f),
      width(SPRITE_SIZE), height(SPRITE_SIZE),
      currentDirection(DIR_DOWN),
      moving(false),
      animTime(0.0f),
      animFrame(0),
      texMoveUp(nullptr), texMoveDown(nullptr),
      texMoveLeft(nullptr), texMoveRight(nullptr),
      indicatorTexture(nullptr), nameFont(nullptr)
{
    for (int i = 0; i < SDL_NUM_SCANCODES; ++i) keys[i] = false;

    for (int i = 0; i < WALK_FRAMES; ++i) {
        texWalkUp[i]    = nullptr;
        texWalkDown[i]  = nullptr;
        texWalkLeft[i]  = nullptr;
        texWalkRight[i] = nullptr;
    }

    setSpriteDirectory(renderer, "resources/sprites/characters/player");

    SDL_Surface* indicator = resource_archive::loadResourceImage(
        "resources/interfaces/indicators/indicator_player.png");
    if (indicator) {
        indicatorTexture = SDL_CreateTextureFromSurface(renderer, indicator);
        SDL_FreeSurface(indicator);
        if (!indicatorTexture) {
            std::cerr << "Falha ao criar textura do indicador do jogador: "
                      << SDL_GetError() << std::endl;
        } else {
            SDL_SetTextureScaleMode(indicatorTexture, SDL_ScaleModeNearest);
        }
    } else {
        std::cerr << "Falha ao carregar indicator_player.png: "
                  << IMG_GetError() << std::endl;
    }

    if (TTF_WasInit() == 0 && TTF_Init() < 0) {
        std::cerr << "TTF_Init falhou para indicador do jogador: "
                  << TTF_GetError() << std::endl;
    } else {
        nameFont = resource_archive::openResourceFont(
            "resources/fonts/impact.ttf", 28);
        if (!nameFont) {
            std::cerr << "Falha ao carregar fonte do indicador do jogador: "
                      << TTF_GetError() << std::endl;
        }
    }
}

Player::~Player() {
    destroySpriteTextures();
    if (indicatorTexture) SDL_DestroyTexture(indicatorTexture);
    if (nameFont) TTF_CloseFont(nameFont);
}

void Player::initializeHitPoints() {
    const int constitutionModifier =
        static_cast<int>(std::floor((constitution - 10) / 2.0));
    const int boundedLevel = std::max(1, std::min(level, MAX_LEVEL));
    maxHp = std::max(1, 10 + constitutionModifier);
    hp = maxHp;
    for (int currentLevel = 2; currentLevel <= boundedLevel; ++currentLevel) {
        const int gainedHp =
            std::max(1, Dice1d10::roll() + constitutionModifier);
        maxHp += gainedHp;
        hp += gainedHp;
    }
}

bool Player::gainLevel() {
    if (level >= MAX_LEVEL) return false;

    const int constitutionModifier =
        static_cast<int>(std::floor((constitution - 10) / 2.0));
    const int gainedHp =
        std::max(1, Dice1d10::roll() + constitutionModifier);
    ++level;
    maxHp += gainedHp;
    hp = std::min(maxHp, hp + gainedHp);
    return true;
}

int Player::experienceToNextLevel() const {
    return level >= MAX_LEVEL ? 0 : level * 2500;
}

int Player::gainExperience(int amount) {
    if (amount <= 0) return 0;
    experience += amount;
    int levelsGained = 0;
    while (level < MAX_LEVEL && experience >= experienceToNextLevel()) {
        experience -= experienceToNextLevel();
        if (!gainLevel()) break;
        ++levelsGained;
    }
    return levelsGained;
}

void Player::clearMovementInput() {
    for (int i = 0; i < SDL_NUM_SCANCODES; ++i) keys[i] = false;
    moving = false;
    animTime = 0.0f;
    animFrame = 0;
}

void Player::destroySpriteTextures() {
    if (texMoveUp)    SDL_DestroyTexture(texMoveUp);
    if (texMoveDown)  SDL_DestroyTexture(texMoveDown);
    if (texMoveLeft)  SDL_DestroyTexture(texMoveLeft);
    if (texMoveRight) SDL_DestroyTexture(texMoveRight);

    for (int i = 0; i < WALK_FRAMES; ++i) {
        if (texWalkUp[i])    SDL_DestroyTexture(texWalkUp[i]);
        if (texWalkDown[i])  SDL_DestroyTexture(texWalkDown[i]);
        if (texWalkLeft[i])  SDL_DestroyTexture(texWalkLeft[i]);
        if (texWalkRight[i]) SDL_DestroyTexture(texWalkRight[i]);
    }
}

std::vector<SDL_Texture**> Player::spriteTextureDestinations() {
    return {
        &texMoveUp, &texMoveDown, &texMoveLeft, &texMoveRight,
        &texWalkUp[0], &texWalkUp[1], &texWalkUp[2],
        &texWalkDown[0], &texWalkDown[1], &texWalkDown[2],
        &texWalkLeft[0], &texWalkLeft[1], &texWalkLeft[2],
        &texWalkRight[0], &texWalkRight[1], &texWalkRight[2]
    };
}

bool Player::setSpriteDirectory(SDL_Renderer* renderer,
                                const std::string& directory) {
    static const std::string allowedPrefix =
        "resources/sprites/characters/player";
    if (directory != allowedPrefix &&
        (directory.compare(0, allowedPrefix.size(), allowedPrefix) != 0 ||
         directory.size() <= allowedPrefix.size() ||
         directory[allowedPrefix.size()] != '/' ||
         directory.find("..") != std::string::npos)) {
        std::cerr << "Diretorio de sprites do jogador invalido: "
                  << directory << std::endl;
        return false;
    }

    const std::vector<std::string> filenames = {
        "stop_up.png", "stop_down.png", "stop_left.png", "stop_right.png",
        "walk_up_01.png", "stop_up.png", "walk_up_02.png", 
        "walk_down_01.png", "stop_down.png", "walk_down_02.png",
        "walk_left_01.png", "stop_left.png", "walk_left_02.png",
        "walk_right_01.png", "stop_right.png", "walk_right_02.png"
    };
    std::vector<std::string> paths;
    paths.reserve(filenames.size());
    for (const std::string& filename : filenames) {
        const std::string path = directory + "/" + filename;
        SDL_Surface* image = resource_archive::loadResourceImage(path);
        if (!image) {
            std::cerr << "Sprites incompletos para o personagem em "
                      << directory << ": arquivo ausente " << path
                      << ". O conjunto de sprites atual sera mantido."
                      << std::endl;
            return false;
        }
        SDL_FreeSurface(image);
        paths.push_back(path);
    }

    std::vector<SDL_Texture*> loadedTextures(paths.size(), nullptr);
    for (std::size_t i = 0; i < paths.size(); ++i) {
        if (!loadTexture(renderer, paths[i], loadedTextures[i])) {
            for (SDL_Texture* texture : loadedTextures) {
                if (texture) SDL_DestroyTexture(texture);
            }
            return false;
        }
    }

    destroySpriteTextures();
    const std::vector<SDL_Texture**> destinations =
        spriteTextureDestinations();
    for (std::size_t i = 0; i < destinations.size(); ++i) {
        *destinations[i] = loadedTextures[i];
    }
    spritePath = directory;
    return true;
}

bool Player::loadTexture(SDL_Renderer* renderer,
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

void Player::getHitbox(float& hx, float& hy, int& hw, int& hh) const {
    hw = width  * 7 / 10;
    hh = height * 7 / 10;
    hx = x - hw / 2.0f;
    hy = y - hh / 2.0f + (height - hh) / 3.0f;
}

void Player::handleEvent(const SDL_Event& event) {
    if (event.type == SDL_KEYDOWN) {
        if (event.key.keysym.scancode == SDL_SCANCODE_ESCAPE) {
            SDL_Event quitEvent;
            quitEvent.type = SDL_QUIT;
            SDL_PushEvent(&quitEvent);
            return;
        }
        keys[event.key.keysym.scancode] = true;
    }
    else if (event.type == SDL_KEYUP) {
        keys[event.key.keysym.scancode] = false;
    }
}

void Player::update(float dt, const CavesOfChaos& map) {
    float dx = 0.0f, dy = 0.0f;

    if (keys[SDL_SCANCODE_UP])    { dy -= 1.0f; currentDirection = DIR_UP;    }
    if (keys[SDL_SCANCODE_DOWN])  { dy += 1.0f; currentDirection = DIR_DOWN;  }
    if (keys[SDL_SCANCODE_LEFT])  { dx -= 1.0f; currentDirection = DIR_LEFT;  }
    if (keys[SDL_SCANCODE_RIGHT]) { dx += 1.0f; currentDirection = DIR_RIGHT; }

    moving = (dx != 0.0f || dy != 0.0f);

    if (dx != 0.0f && dy != 0.0f) {
        float len = std::sqrt(dx * dx + dy * dy);
        dx /= len;
        dy /= len;
    }

    float newX = x + dx * speed * dt;
    {
        float hx, hy; int hw, hh;
        getHitbox(hx, hy, hw, hh);
        float testX = hx + (newX - x);
        if (!map.collides(testX, hy, hw, hh) &&
            !map.collidesWithEnemies(testX, hy, hw, hh)) {
            x = newX;
        }
    }

    float newY = y + dy * speed * dt;
    {
        float hx, hy; int hw, hh;
        getHitbox(hx, hy, hw, hh);
        float testY = hy + (newY - y);
        if (!map.collides(hx, testY, hw, hh) &&
            !map.collidesWithEnemies(hx, testY, hw, hh)) {
            y = newY;
        }
    }

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

void Player::render(SDL_Renderer* renderer, int camX, int camY) {
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
            case DIR_UP:    tex = texMoveUp;    break;
            case DIR_DOWN:  tex = texMoveDown;  break;
            case DIR_LEFT:  tex = texMoveLeft;  break;
            case DIR_RIGHT: tex = texMoveRight; break;
        }
    }

    if (!tex) return;

    SDL_Rect dst;
    dst.w = SPRITE_SIZE;
    dst.h = SPRITE_SIZE;
    dst.x = static_cast<int>(x) - SPRITE_SIZE / 2 - camX;
    dst.y = static_cast<int>(y) - SPRITE_SIZE / 2 - camY;

    SDL_RenderCopy(renderer, tex, nullptr, &dst);
}

void Player::renderNameplate(SDL_Renderer* renderer, int camX, int camY) const {
    if (!indicatorTexture) return;

    int indicatorWidth = 150;
    int indicatorHeight = 45;
    SDL_QueryTexture(indicatorTexture, nullptr, nullptr,
                     &indicatorWidth, &indicatorHeight);
    const int screenX = static_cast<int>(x) - camX;
    const int screenY = static_cast<int>(y) - camY;
    const SDL_Rect indicatorRect = {
        screenX - indicatorWidth / 2,
        screenY - SPRITE_SIZE / 2 - indicatorHeight,
        indicatorWidth,
        indicatorHeight
    };
    SDL_RenderCopy(renderer, indicatorTexture, nullptr, &indicatorRect);

    if (!nameFont || name.empty()) return;
    int textWidth = 0;
    int textHeight = 0;
    if (TTF_SizeUTF8(nameFont, name.c_str(), &textWidth, &textHeight) < 0) {
        std::cerr << "Falha ao dimensionar nome do jogador: "
                  << TTF_GetError() << std::endl;
        return;
    }

    constexpr int horizontalPadding = 12;
    constexpr int verticalPadding = 6;
    const int maxWidth = indicatorWidth - horizontalPadding * 2;
    const int maxHeight = indicatorHeight - verticalPadding * 2;
    const double scale = std::min(
        1.0, std::min(static_cast<double>(maxWidth) /
                          std::max(1, textWidth),
                      static_cast<double>(maxHeight) /
                          std::max(1, textHeight)));
    const int fittedWidth = std::max(
        1, static_cast<int>(textWidth * scale));
    const int fittedHeight = std::max(
        1, static_cast<int>(textHeight * scale));

    const SDL_Color nameColor = {0xFF, 0xC6, 0x00, 255};
    SDL_Surface* textSurface = TTF_RenderUTF8_Blended(
        nameFont, name.c_str(), nameColor);
    if (!textSurface) {
        std::cerr << "Falha ao renderizar nome do jogador: "
                  << TTF_GetError() << std::endl;
        return;
    }
    SDL_Texture* textTexture =
        SDL_CreateTextureFromSurface(renderer, textSurface);
    SDL_FreeSurface(textSurface);
    if (!textTexture) {
        std::cerr << "Falha ao criar textura do nome do jogador: "
                  << SDL_GetError() << std::endl;
        return;
    }

    const SDL_Rect textRect = {
        indicatorRect.x + (indicatorWidth - fittedWidth) / 2,
        indicatorRect.y + (indicatorHeight - fittedHeight) / 2,
        fittedWidth,
        fittedHeight
    };
    SDL_RenderCopy(renderer, textTexture, nullptr, &textRect);
    SDL_DestroyTexture(textTexture);
}
#include "scripts/itens/potion_hp_minor.h"
#include "scripts/resources/resource_archive.h"
#include "scripts/characters/player.h"
#include "scripts/dice/dice_1d4.h"
#include <iostream>

PotionHpMinor::PotionHpMinor()
    : spriteTex(nullptr), spriteW(0), spriteH(0),
      descTex(nullptr), descW(0), descH(0)
{
}

PotionHpMinor::~PotionHpMinor() {
    if (spriteTex) SDL_DestroyTexture(spriteTex);
    if (descTex)   SDL_DestroyTexture(descTex);
}

bool PotionHpMinor::loadTexture(SDL_Renderer* renderer,
                                const std::string& path,
                                SDL_Texture*& out,
                                int& outW, int& outH)
{
    SDL_Surface* surf = resource_archive::loadResourceImage(path);
    if (!surf) {
        std::cerr << "Falha ao carregar " << path
                  << ": " << IMG_GetError() << std::endl;
        return false;
    }

    out = SDL_CreateTextureFromSurface(renderer, surf);
    if (!out) {
        std::cerr << "Falha ao criar textura de " << path
                  << ": " << SDL_GetError() << std::endl;
        SDL_FreeSurface(surf);
        return false;
    }

    outW = surf->w;
    outH = surf->h;
    SDL_FreeSurface(surf);

    SDL_SetTextureScaleMode(out, SDL_ScaleModeNearest);
    return true;
}

bool PotionHpMinor::load(SDL_Renderer* renderer) {
    if (TTF_Init() < 0) {
        std::cerr << "TTF_Init falhou: " << TTF_GetError() << std::endl;
        return false;
    }

    if (!loadTexture(renderer,
                     "resources/sprites/objects/potion/potion_hp_minor_02.png",
                     spriteTex, spriteW, spriteH)) {
        return false;
    }

    TTF_Font* font = resource_archive::openResourceFont("resources/fonts/LifeCraft.ttf", 18);
    if (!font) {
        std::cerr << "Falha ao carregar fonte: " << TTF_GetError() << std::endl;
        return false;
    }

    SDL_Color color = { 255, 255, 255, 255 };
    SDL_Surface* textSurf = TTF_RenderText_Blended(
        font, "Roll 2d4+2 to heal your HP for 1 turn.", color);

    TTF_CloseFont(font);
    font = nullptr;

    if (!textSurf) {
        std::cerr << "Falha ao renderizar descricao: "
                  << TTF_GetError() << std::endl;
        return false;
    }

    descTex = SDL_CreateTextureFromSurface(renderer, textSurf);
    descW = textSurf->w;
    descH = textSurf->h;
    SDL_FreeSurface(textSurf);

    if (!descTex) {
        std::cerr << "Falha ao criar textura de descricao: "
                  << SDL_GetError() << std::endl;
        return false;
    }

    SDL_SetTextureScaleMode(descTex, SDL_ScaleModeLinear);
    return true;
}

void PotionHpMinor::renderSprite(SDL_Renderer* renderer, int x, int y) const {
    if (!spriteTex) return;
    SDL_Rect dst = { x, y, spriteW, spriteH };
    SDL_RenderCopy(renderer, spriteTex, nullptr, &dst);
}

void PotionHpMinor::renderSpriteScaled(SDL_Renderer* renderer,
                                       int x, int y, int w, int h) const
{
    if (!spriteTex) return;
    SDL_Rect dst = { x, y, w, h };
    SDL_RenderCopy(renderer, spriteTex, nullptr, &dst);
}

void PotionHpMinor::renderDescription(SDL_Renderer* renderer, int x, int y) const {
    if (!descTex) return;
    SDL_Rect dst = { x, y, descW, descH };
    SDL_RenderCopy(renderer, descTex, nullptr, &dst);
}

void PotionHpMinor::use(Player& player) const {
    int r1 = Dice1d4::roll();
    int r2 = Dice1d4::roll();
    int heal = r1 + r2 + 2;

    player.hp += heal;
    if (player.hp > player.maxHp) {
        player.hp = player.maxHp;
    }

    std::cout << "Potion HP Minor usada: "
              << r1 << " + " << r2 << " + 2 = " << heal
              << " HP curados. HP: " << player.hp << "/" << player.maxHp
              << std::endl;
}
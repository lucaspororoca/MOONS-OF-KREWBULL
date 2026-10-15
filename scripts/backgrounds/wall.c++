#include "scripts/backgrounds/wall.h"
#include "scripts/resources/resource_archive.h"
#include <iostream>

Wall::Wall() {}

Wall::~Wall() {
    for (auto& pair : textures) {
        if (pair.second) SDL_DestroyTexture(pair.second);
    }
    textures.clear();
}

bool Wall::loadTexture(SDL_Renderer* renderer,
                       const std::string& path,
                       SDL_Texture*& out)
{
    SDL_Surface* surf = resource_archive::loadResourceImage(path);
    if (!surf) {
        std::cerr << "Falha ao carregar " << path
                  << ": " << IMG_GetError() << std::endl;
        return false;
    }

    SDL_Surface* scaled = SDL_CreateRGBSurfaceWithFormat(
        0, TILE_SIZE, TILE_SIZE, 32, SDL_PIXELFORMAT_RGBA32);
    if (!scaled) {
        std::cerr << "Falha ao criar surface escalada: "
                  << SDL_GetError() << std::endl;
        SDL_FreeSurface(surf);
        return false;
    }

    SDL_SetSurfaceBlendMode(surf, SDL_BLENDMODE_BLEND);
    SDL_BlitScaled(surf, nullptr, scaled, nullptr);
    SDL_SetSurfaceBlendMode(scaled, SDL_BLENDMODE_BLEND);

    out = SDL_CreateTextureFromSurface(renderer, scaled);
    SDL_FreeSurface(scaled);
    SDL_FreeSurface(surf);

    if (!out) {
        std::cerr << "Falha ao criar textura de " << path
                  << ": " << SDL_GetError() << std::endl;
        return false;
    }

    SDL_SetTextureScaleMode(out, SDL_ScaleModeNearest);
    return true;
}

bool Wall::load(SDL_Renderer* renderer) {
    SDL_Texture* tex1 = nullptr;
    SDL_Texture* tex2 = nullptr;
    SDL_Texture* tex3 = nullptr;
    if (!loadTexture(renderer,
                     "resources/sprites/backgrounds/wall/wall_01.png",
                     tex1)) {
        return false;
    }
    if (!loadTexture(renderer,
                     "resources/sprites/backgrounds/wall/wall_02.png",
                     tex2)) {
        return false;
    }
     if (!loadTexture(renderer,
                     "resources/sprites/backgrounds/wall/wall_03.png",
                     tex3)) {
        return false;
    }
    textures[1] = tex1;
    textures[2] = tex2;
    textures[3] = tex3;

    return true;
}

SDL_Texture* Wall::getTexture(int id) const {
    auto it = textures.find(id);
    if (it != textures.end()) return it->second;
    return nullptr;
}
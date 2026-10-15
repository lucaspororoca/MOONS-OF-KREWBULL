#include "scripts/objects/boockcase.h"
#include "scripts/resources/resource_archive.h"
#include <iostream>

Boockcase::Boockcase()
    : part1Texture(nullptr), part2Texture(nullptr) {}

Boockcase::~Boockcase() {
    if (part1Texture) SDL_DestroyTexture(part1Texture);
    if (part2Texture) SDL_DestroyTexture(part2Texture);
}

bool Boockcase::loadTexture(SDL_Renderer* renderer, const char* path,
                            SDL_Texture*& outTexture) {
    SDL_Surface* source = resource_archive::loadResourceImage(path);
    if (!source) {
        std::cerr << "Falha ao carregar " << path << ": "
                  << IMG_GetError() << std::endl;
        return false;
    }

    SDL_Surface* scaled = SDL_CreateRGBSurfaceWithFormat(
        0, TILE_SIZE, TILE_SIZE, 32, SDL_PIXELFORMAT_RGBA32);
    if (!scaled) {
        std::cerr << "Falha ao criar surface para " << path << ": "
                  << SDL_GetError() << std::endl;
        SDL_FreeSurface(source);
        return false;
    }

    SDL_SetSurfaceBlendMode(source, SDL_BLENDMODE_BLEND);
    if (SDL_BlitScaled(source, nullptr, scaled, nullptr) < 0) {
        std::cerr << "Falha ao redimensionar " << path << ": "
                  << SDL_GetError() << std::endl;
        SDL_FreeSurface(source);
        SDL_FreeSurface(scaled);
        return false;
    }

    outTexture = SDL_CreateTextureFromSurface(renderer, scaled);
    SDL_FreeSurface(source);
    SDL_FreeSurface(scaled);
    if (!outTexture) {
        std::cerr << "Falha ao criar textura de " << path << ": "
                  << SDL_GetError() << std::endl;
        return false;
    }

    SDL_SetTextureScaleMode(outTexture, SDL_ScaleModeNearest);
    return true;
}

bool Boockcase::load(SDL_Renderer* renderer) {
    if (!loadTexture(renderer,
            "resources/sprites/objects/bookcase/boockcase_01_part1.png",
            part1Texture)) {
        return false;
    }
    if (!loadTexture(renderer,
            "resources/sprites/objects/bookcase/boockcase_01_part2.png",
            part2Texture)) {
        SDL_DestroyTexture(part1Texture);
        part1Texture = nullptr;
        return false;
    }
    return true;
}

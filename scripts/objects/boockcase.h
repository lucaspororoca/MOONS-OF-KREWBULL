#ifndef BOOCKCASE_H
#define BOOCKCASE_H

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>

class Boockcase {
public:
    static const int TILE_SIZE = 176;

    Boockcase();
    ~Boockcase();

    bool load(SDL_Renderer* renderer);
    SDL_Texture* getPart1Texture() const { return part1Texture; }
    SDL_Texture* getPart2Texture() const { return part2Texture; }

    bool isPart1(char tile) const { return tile == 's'; }
    bool isPart2(char tile) const { return tile == 'S'; }

private:
    bool loadTexture(SDL_Renderer* renderer, const char* path,
                     SDL_Texture*& outTexture);

    SDL_Texture* part1Texture;
    SDL_Texture* part2Texture;
};

#endif

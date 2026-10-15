#ifndef CHAIR_H
#define CHAIR_H

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <string>
#include <unordered_map>

class Chair {
public:
    static const int TILE_SIZE = 176;

    Chair();
    ~Chair();

    bool load(SDL_Renderer* renderer);
    SDL_Texture* getTexture(int id) const;

    bool isChair(char c) const;
    int  charToId(char c) const;

private:
    bool loadTexture(SDL_Renderer* renderer,
                     const std::string& path,
                     SDL_Texture*& out);

    std::unordered_map<int, SDL_Texture*> textures;
};

#endif
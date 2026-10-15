#ifndef WALL_H
#define WALL_H

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <string>
#include <unordered_map>

class Wall {
public:
    static const int TILE_SIZE = 176;

    Wall();
    ~Wall();

    bool load(SDL_Renderer* renderer);
    SDL_Texture* getTexture(int id) const;

private:
    bool loadTexture(SDL_Renderer* renderer,
                     const std::string& path,
                     SDL_Texture*& out);

    std::unordered_map<int, SDL_Texture*> textures;
};

#endif // WALL_H
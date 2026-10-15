#ifndef FLOOR_H
#define FLOOR_H

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <string>
#include <unordered_map>

class Floor {
public:
    static const int TILE_SIZE = 176;

    Floor();
    ~Floor();

    bool load(SDL_Renderer* renderer);
    SDL_Texture* getTexture(int id) const;

private:
    bool loadTexture(SDL_Renderer* renderer,
                     const std::string& path,
                     SDL_Texture*& out);

    std::unordered_map<int, SDL_Texture*> textures;
};

#endif // FLOOR_H
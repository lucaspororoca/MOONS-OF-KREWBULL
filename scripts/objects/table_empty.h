#ifndef TABLE_EMPTY_H
#define TABLE_EMPTY_H

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <string>
#include <unordered_map>

class TableEmpty {
public:
    static const int TILE_SIZE = 176;

    TableEmpty();
    ~TableEmpty();

    bool load(SDL_Renderer* renderer);
    SDL_Texture* getTexture(int id) const;

    bool isTable(char c) const;
    int  charToId(char c) const;

private:
    bool loadTexture(SDL_Renderer* renderer,
                     const std::string& path,
                     SDL_Texture*& out);

    std::unordered_map<int, SDL_Texture*> textures;
};

#endif
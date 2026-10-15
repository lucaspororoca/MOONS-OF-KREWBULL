#ifndef TABLE_PAPER_H
#define TABLE_PAPER_H

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <string>
#include <unordered_map>
#include <vector>

class Player;
class Bag;

class TablePaper {
public:
    static const int TILE_SIZE = 176;

    TablePaper();
    ~TablePaper();

    bool load(SDL_Renderer* renderer);
    SDL_Texture* getTexture(int id) const;

    bool isTable(char c) const;
    int  charToId(char c) const;

    bool findActiveTable(const std::vector<std::string>& tiles,
                         int rows, int cols,
                         float px, float py, int pw, int ph,
                         int& outRow, int& outCol) const;

    bool collect(std::vector<std::string>& tiles,
                 int row, int col,
                 Bag& bag) const;

    void renderPrompt(SDL_Renderer* renderer, int camX, int camY,
                      const Player& player) const;

private:
    bool loadTexture(SDL_Renderer* renderer,
                     const std::string& path,
                     SDL_Texture*& out);
    bool loadTextureNative(SDL_Renderer* renderer,
                           const std::string& path,
                           SDL_Texture*& out,
                           int& outW, int& outH);

    std::unordered_map<int, SDL_Texture*> textures;

    SDL_Texture* noticeOptionCollectTex;
    int noticeOptionCollectW;
    int noticeOptionCollectH;
};

#endif
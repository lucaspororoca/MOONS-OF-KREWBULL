#include "scripts/objects/table_paper.h"
#include "scripts/resources/resource_archive.h"
#include "scripts/characters/player.h"
#include "scripts/hud/bag.h"
#include <iostream>

TablePaper::TablePaper()
    : noticeOptionCollectTex(nullptr),
      noticeOptionCollectW(0),
      noticeOptionCollectH(0)
{
}

TablePaper::~TablePaper() {
    for (auto& pair : textures) {
        if (pair.second) SDL_DestroyTexture(pair.second);
    }
    textures.clear();

    if (noticeOptionCollectTex) SDL_DestroyTexture(noticeOptionCollectTex);
}

bool TablePaper::loadTexture(SDL_Renderer* renderer,
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

bool TablePaper::loadTextureNative(SDL_Renderer* renderer,
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

bool TablePaper::load(SDL_Renderer* renderer) {
    SDL_Texture* tex1 = nullptr;
    if (!loadTexture(renderer,
                     "resources/sprites/objects/table/table_01_paper.png",
                     tex1)) {
        return false;
    }
    textures[1] = tex1;

    loadTextureNative(renderer,
        "resources/interfaces/notices/notice_option_collect.png",
        noticeOptionCollectTex,
        noticeOptionCollectW,
        noticeOptionCollectH);

    return true;
}

SDL_Texture* TablePaper::getTexture(int id) const {
    auto it = textures.find(id);
    if (it != textures.end()) return it->second;
    return nullptr;
}

bool TablePaper::isTable(char c) const {
    return c == 'd';
}

int TablePaper::charToId(char c) const {
    if (!isTable(c)) return -1;
    return 1;
}

bool TablePaper::findActiveTable(const std::vector<std::string>& tiles,
                                 int rows, int cols,
                                 float px, float py, int pw, int ph,
                                 int& outRow, int& outCol) const
{
    const float margin = 16.0f;

    int left   = static_cast<int>((px - margin) / TILE_SIZE);
    int top    = static_cast<int>((py - margin) / TILE_SIZE);
    int right  = static_cast<int>((px + pw + margin) / TILE_SIZE);
    int bottom = static_cast<int>((py + ph + margin) / TILE_SIZE);

    if (left < 0) left = 0;
    if (top < 0) top = 0;
    if (right >= cols) right = cols - 1;
    if (bottom >= rows) bottom = rows - 1;

    for (int ty = top; ty <= bottom; ++ty) {
        for (int tx = left; tx <= right; ++tx) {
            if (isTable(tiles[ty][tx])) {
                outRow = ty;
                outCol = tx;
                return true;
            }
        }
    }
    return false;
}

bool TablePaper::collect(std::vector<std::string>& tiles,
                         int row, int col,
                         Bag& bag) const
{
    if (row < 0 || col < 0) return false;
    if (row >= static_cast<int>(tiles.size())) return false;
    if (col >= static_cast<int>(tiles[row].size())) return false;
    if (!isTable(tiles[row][col])) return false;

    bag.addItem("Paper", 0.01f, 1);

    tiles[row][col] = 'c';
    return true;
}

void TablePaper::renderPrompt(SDL_Renderer* renderer, int camX, int camY,
                              const Player& player) const
{
    if (!noticeOptionCollectTex) return;

    const int GAP = 10;
    const int PLAYER_SPRITE = Player::SPRITE_SIZE;

    int playerScreenX = static_cast<int>(player.getX()) - camX;
    int playerScreenY = static_cast<int>(player.getY()) - camY;

    SDL_Rect dst;
    dst.w = noticeOptionCollectW;
    dst.h = noticeOptionCollectH;
    dst.x = playerScreenX + PLAYER_SPRITE / 2 + GAP;
    dst.y = playerScreenY - noticeOptionCollectH / 2;

    SDL_RenderCopy(renderer, noticeOptionCollectTex, nullptr, &dst);
}
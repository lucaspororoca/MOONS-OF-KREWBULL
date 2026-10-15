#ifndef SCREEN_LOAD_H
#define SCREEN_LOAD_H

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_ttf.h>
#include <filesystem>
#include <string>
#include <vector>

#include "scripts/resources/player_save.h"

class TelaLoad {
public:
    TelaLoad(SDL_Renderer* renderer, int screenW, int screenH);
    ~TelaLoad();

    bool init();
    void handleEvent(const SDL_Event& event);
    void update(float dt);
    void render();

    bool isRunning() const { return running; }
    bool shouldStartGame() const { return startGame; }
    bool shouldReturnToIntro() const { return returnToIntro; }
    const std::filesystem::path& selectedSavePath() const {
        return selectedPath;
    }

private:
    struct SaveEntry {
        std::filesystem::path path;
        player_save::Character character;
        bool hovered = false;
    };

    bool loadTextureFixed(const std::string& path,
                          SDL_Texture*& out, int width, int height);
    bool loadButton(const std::string& path);
    bool isPixelOpaque(int x, int y) const;
    bool isPointOverButton(int mx, int my, int y) const;
    void renderText(const std::string& text, int x, int y, SDL_Color color);
    int visibleEntryCount() const;

    SDL_Renderer* renderer;
    int screenW;
    int screenH;
    bool running;
    bool startGame;
    bool returnToIntro;

    SDL_Texture* backgroundTexture;
    SDL_Texture* loadButtonTexture;
    SDL_Surface* loadButtonMask;
    TTF_Font* font;

    std::vector<SaveEntry> entries;
    std::size_t firstVisible;
    std::filesystem::path selectedPath;
    std::string statusMessage;
};

#endif

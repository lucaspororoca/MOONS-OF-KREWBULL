#ifndef SCREEN_GAMEOVER_H
#define SCREEN_GAMEOVER_H

#include <SDL2/SDL.h>
#include <string>

class TelaGameOver {
public:
    enum class Choice {
        NONE,
        REVIVE,
        RETURN_TO_MENU
    };

    TelaGameOver(SDL_Renderer* renderer, int screenW, int screenH,
                 const std::string& characterClass,
                 const std::string& gender);
    ~TelaGameOver();

    bool init();
    void handleEvent(const SDL_Event& event);
    void update(float dt);
    void render();

    bool isRunning() const { return running; }
    Choice choice() const { return selectedChoice; }

private:
    bool loadFullscreen(const std::string& path, SDL_Texture*& texture);
    bool loadButton(const std::string& path, SDL_Texture*& texture,
                    SDL_Surface*& mask);
    bool isOpaque(const SDL_Surface* surface, int x, int y) const;
    bool hitButton(const SDL_Surface* mask, int mouseX, int mouseY) const;
    std::string backgroundPath() const;

    SDL_Renderer* renderer;
    int screenW;
    int screenH;
    std::string characterClass;
    std::string gender;
    bool running;
    Choice selectedChoice;

    SDL_Texture* backgroundTexture;
    SDL_Texture* reviveTexture;
    SDL_Texture* menuTexture;
    SDL_Surface* reviveMask;
    SDL_Surface* menuMask;
};

#endif

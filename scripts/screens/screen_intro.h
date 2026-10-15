#ifndef SCREEN_INTRO_H
#define SCREEN_INTRO_H

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <string>

class TelaIntro {
public:
    TelaIntro(SDL_Renderer* renderer, int screenW, int screenH);
    ~TelaIntro();

    bool init();
    void handleEvent(const SDL_Event& event);
    void update(float dt);
    void render();

    bool isRunning() const { return running; }
    bool shouldStartGame() const { return startGame; }
    bool shouldOpenLoadScreen() const { return openLoadScreen; }
    void stop() { running = false; }

private:
    bool loadTextureFixed(const std::string& path,
                          SDL_Texture*& out,
                          int w, int h);
    bool loadButtonNative(const std::string& path,
                          SDL_Texture*& out,
                          SDL_Surface*& mask);

    bool isPixelOpaque(SDL_Surface* surf, int x, int y) const;
    bool isPointOverButton(int mx, int my) const;
    bool isPointOverLoadButton(int mx, int my) const;
    void updateHover();

    SDL_Renderer* renderer;
    int screenW;
    int screenH;
    bool running;
    bool startGame;

    SDL_Texture* backgroundTex;
    SDL_Texture* buttonIdleTex;
    SDL_Texture* buttonHoverTex;
    SDL_Surface* buttonMask;
    SDL_Texture* loadButtonIdleTex;
    SDL_Texture* loadButtonHoverTex;
    SDL_Surface* loadButtonMask;

    SDL_Rect buttonRect;
    SDL_Rect loadButtonRect;

    bool buttonHovered;
    bool loadButtonHovered;
    bool openLoadScreen;
};

#endif
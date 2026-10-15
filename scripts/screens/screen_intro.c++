#include "scripts/screens/screen_intro.h"
#include "scripts/resources/resource_archive.h"
#include "scripts/hud/bgm_manager.h"
#include <iostream>

TelaIntro::TelaIntro(SDL_Renderer* renderer, int screenW, int screenH)
    : renderer(renderer),
      screenW(screenW),
      screenH(screenH),
      running(true),
      startGame(false),
      backgroundTex(nullptr),
      buttonIdleTex(nullptr),
      buttonHoverTex(nullptr),
      buttonMask(nullptr),
      loadButtonIdleTex(nullptr),
      loadButtonHoverTex(nullptr),
      loadButtonMask(nullptr),
      buttonHovered(false),
      loadButtonHovered(false),
      openLoadScreen(false)
{
    buttonRect = { 0, 0, screenW, screenH };
    loadButtonRect = { 0, 0, screenW, screenH };
}

TelaIntro::~TelaIntro() {
    if (backgroundTex)  SDL_DestroyTexture(backgroundTex);
    if (buttonIdleTex)  SDL_DestroyTexture(buttonIdleTex);
    if (buttonHoverTex) SDL_DestroyTexture(buttonHoverTex);
    if (buttonMask)     SDL_FreeSurface(buttonMask);
    if (loadButtonIdleTex) SDL_DestroyTexture(loadButtonIdleTex);
    if (loadButtonHoverTex) SDL_DestroyTexture(loadButtonHoverTex);
    if (loadButtonMask) SDL_FreeSurface(loadButtonMask);
}

bool TelaIntro::loadTextureFixed(const std::string& path,
                                 SDL_Texture*& out,
                                 int w, int h)
{
    SDL_Surface* surf = resource_archive::loadResourceImage(path);
    if (!surf) {
        std::cerr << "Falha ao carregar " << path
                  << ": " << IMG_GetError() << std::endl;
        return false;
    }

    SDL_Surface* scaled = SDL_CreateRGBSurfaceWithFormat(
        0, w, h, 32, SDL_PIXELFORMAT_RGBA32);
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

bool TelaIntro::loadButtonNative(const std::string& path,
                                 SDL_Texture*& out,
                                 SDL_Surface*& mask)
{
    SDL_Surface* surf = resource_archive::loadResourceImage(path);
    if (!surf) {
        std::cerr << "Falha ao carregar " << path
                  << ": " << IMG_GetError() << std::endl;
        return false;
    }

    SDL_Surface* converted = SDL_ConvertSurfaceFormat(
        surf, SDL_PIXELFORMAT_RGBA32, 0);
    SDL_FreeSurface(surf);

    if (!converted) {
        std::cerr << "Falha ao converter " << path
                  << ": " << SDL_GetError() << std::endl;
        return false;
    }

    out = SDL_CreateTextureFromSurface(renderer, converted);
    if (!out) {
        std::cerr << "Falha ao criar textura de " << path
                  << ": " << SDL_GetError() << std::endl;
        SDL_FreeSurface(converted);
        return false;
    }

    SDL_SetTextureScaleMode(out, SDL_ScaleModeNearest);
    mask = converted;
    return true;
}

bool TelaIntro::init() {
    if (!loadTextureFixed(
            "resources/interfaces/scenes/intro/scene_intro_01.png",
            backgroundTex, screenW, screenH)) {
        return false;
    }

    if (!loadButtonNative(
            "resources/interfaces/buttons/button_start_game_idle.png",
            buttonIdleTex, buttonMask)) {
        return false;
    }

    SDL_Surface* unusedMask = nullptr;
    if (!loadButtonNative(
            "resources/interfaces/buttons/button_start_game_hover.png",
            buttonHoverTex, unusedMask)) {
        return false;
    }
    if (unusedMask) SDL_FreeSurface(unusedMask);

    if (!loadButtonNative(
            "resources/interfaces/buttons/button_load_idle.png",
            loadButtonIdleTex, loadButtonMask)) {
        return false;
    }
    unusedMask = nullptr;
    if (!loadButtonNative(
            "resources/interfaces/buttons/button_load_hover.png",
            loadButtonHoverTex, unusedMask)) {
        return false;
    }
    if (unusedMask) SDL_FreeSurface(unusedMask);
    loadButtonRect = {0, 0, screenW, screenH};

    bgm_manager::playLoop("resources/audio/bgm/intro/intro_01.mp3");

    return true;
}

bool TelaIntro::isPixelOpaque(SDL_Surface* surf, int x, int y) const {
    if (!surf) return false;
    if (x < 0 || y < 0 || x >= surf->w || y >= surf->h) return false;

    Uint32* pixels = static_cast<Uint32*>(surf->pixels);
    Uint32 pixel = pixels[y * surf->w + x];
    Uint8 a = static_cast<Uint8>(
        (pixel & surf->format->Amask) >> surf->format->Ashift);
    return a > 10;
}

bool TelaIntro::isPointOverButton(int mx, int my) const {
    if (mx < buttonRect.x || mx >= buttonRect.x + buttonRect.w) return false;
    if (my < buttonRect.y || my >= buttonRect.y + buttonRect.h) return false;

    if (!buttonMask) return true;

    const int px = (mx - buttonRect.x) * buttonMask->w / buttonRect.w;
    const int py = (my - buttonRect.y) * buttonMask->h / buttonRect.h;
    return isPixelOpaque(buttonMask, px, py);
}

bool TelaIntro::isPointOverLoadButton(int mx, int my) const {
    if (mx < loadButtonRect.x || mx >= loadButtonRect.x + loadButtonRect.w ||
        my < loadButtonRect.y || my >= loadButtonRect.y + loadButtonRect.h) {
        return false;
    }
    if (!loadButtonMask) return true;
    const int px = (mx - loadButtonRect.x) * loadButtonMask->w /
                   loadButtonRect.w;
    const int py = (my - loadButtonRect.y) * loadButtonMask->h /
                   loadButtonRect.h;
    return isPixelOpaque(loadButtonMask, px, py);
}

void TelaIntro::updateHover() {
    int mx, my;
    SDL_GetMouseState(&mx, &my);
    buttonHovered = isPointOverButton(mx, my);
    loadButtonHovered = isPointOverLoadButton(mx, my);
}

void TelaIntro::handleEvent(const SDL_Event& event) {
    if (event.type == SDL_QUIT) {
        running = false;
        return;
    }

    if (event.type == SDL_KEYDOWN) {
        if (event.key.keysym.scancode == SDL_SCANCODE_ESCAPE) {
            running = false;
        }
        return;
    }

    if (event.type == SDL_MOUSEBUTTONDOWN &&
        event.button.button == SDL_BUTTON_LEFT) {
        int mx = event.button.x;
        int my = event.button.y;
        if (isPointOverLoadButton(mx, my)) {
            openLoadScreen = true;
        } else if (isPointOverButton(mx, my)) {
            startGame = true;
        }
    }
}

void TelaIntro::update(float dt) {
    (void)dt;
    updateHover();
}

void TelaIntro::render() {
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);

    if (backgroundTex) {
        SDL_Rect dst = { 0, 0, screenW, screenH };
        SDL_RenderCopy(renderer, backgroundTex, nullptr, &dst);
    }

    SDL_Texture* btn = (buttonHovered && buttonHoverTex)
                     ? buttonHoverTex
                     : buttonIdleTex;
    if (btn) {
        SDL_RenderCopy(renderer, btn, nullptr, &buttonRect);
    }
    btn = (loadButtonHovered && loadButtonHoverTex)
        ? loadButtonHoverTex : loadButtonIdleTex;
    if (btn) {
        SDL_RenderCopy(renderer, btn, nullptr, &loadButtonRect);
    }
}
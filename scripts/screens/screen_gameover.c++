#include "scripts/screens/screen_gameover.h"
#include "scripts/resources/resource_archive.h"
#include "scripts/hud/bgm_manager.h"
#include <cstring>
#include <iostream>
#include <map>

TelaGameOver::TelaGameOver(SDL_Renderer* renderer, int screenW, int screenH,
                           const std::string& characterClass,
                           const std::string& gender)
    : renderer(renderer),
      screenW(screenW),
      screenH(screenH),
      characterClass(characterClass),
      gender(gender),
      running(true),
      selectedChoice(Choice::NONE),
      backgroundTexture(nullptr),
      reviveTexture(nullptr),
      menuTexture(nullptr),
      reviveMask(nullptr),
      menuMask(nullptr) {}

TelaGameOver::~TelaGameOver() {
    if (backgroundTexture) SDL_DestroyTexture(backgroundTexture);
    if (reviveTexture) SDL_DestroyTexture(reviveTexture);
    if (menuTexture) SDL_DestroyTexture(menuTexture);
    if (reviveMask) SDL_FreeSurface(reviveMask);
    if (menuMask) SDL_FreeSurface(menuMask);
}

std::string TelaGameOver::backgroundPath() const {
    static const std::map<std::string, std::string> classNames = {
        {"paladin", "paladino"},
        {"paladino", "paladino"},
        {"wizard", "mago"},
        {"ladino", "ladino"},
        {"sorcerer", "feiticeiro"},
        {"sourcerer", "feiticeiro"},
        {"barbarian", "barbaro"}
    };
    const auto className = classNames.find(characterClass);
    const std::string normalizedClass =
        className == classNames.end() ? characterClass : className->second;
    const std::string normalizedGender =
        gender == "female" || gender == "mulher" ? "mulher" : "homem";
    return "resources/interfaces/scenes/game over/gameover_" +
           normalizedClass + "_" + normalizedGender + ".png";
}

bool TelaGameOver::loadFullscreen(const std::string& path,
                                 SDL_Texture*& texture) {
    SDL_Surface* surface = resource_archive::loadResourceImage(path);
    if (!surface) {
        std::cerr << "Falha ao carregar tela de game over " << path << ": "
                  << IMG_GetError() << std::endl;
        return false;
    }
    texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_FreeSurface(surface);
    if (!texture) {
        std::cerr << "Falha ao criar textura de game over " << path << ": "
                  << SDL_GetError() << std::endl;
        return false;
    }
    SDL_SetTextureScaleMode(texture, SDL_ScaleModeNearest);
    return true;
}

bool TelaGameOver::loadButton(const std::string& path,
                              SDL_Texture*& texture, SDL_Surface*& mask) {
    SDL_Surface* source = resource_archive::loadResourceImage(path);
    if (!source) {
        std::cerr << "Falha ao carregar botao de game over " << path << ": "
                  << IMG_GetError() << std::endl;
        return false;
    }
    mask = SDL_ConvertSurfaceFormat(source, SDL_PIXELFORMAT_RGBA32, 0);
    SDL_FreeSurface(source);
    if (!mask) {
        std::cerr << "Falha ao converter mascara do botao " << path << ": "
                  << SDL_GetError() << std::endl;
        return false;
    }
    texture = SDL_CreateTextureFromSurface(renderer, mask);
    if (!texture) {
        std::cerr << "Falha ao criar textura do botao " << path << ": "
                  << SDL_GetError() << std::endl;
        SDL_FreeSurface(mask);
        mask = nullptr;
        return false;
    }
    SDL_SetTextureScaleMode(texture, SDL_ScaleModeNearest);
    return true;
}

bool TelaGameOver::init() {
    const std::string requestedBackground = backgroundPath();
    if (!loadFullscreen(requestedBackground, backgroundTexture)) {
        const std::string fallback =
            "resources/interfaces/scenes/game over/"
            "gameover_paladino_mulher.png";
        if (requestedBackground == fallback ||
            !loadFullscreen(fallback, backgroundTexture)) {
            return false;
        }
        std::cerr << "Fundo especifico indisponivel; usando fallback: "
                  << fallback << std::endl;
    }
    if (!loadButton("resources/interfaces/buttons/button_revive.png",
                    reviveTexture, reviveMask)) {
        return false;
    }
    if (!loadButton(
            "resources/interfaces/scenes/game over/button_back_menu.png",
            menuTexture, menuMask)) {
        std::cerr << "Usando a localizacao existente do botao de menu."
                  << std::endl;
        if (!loadButton("resources/interfaces/buttons/button_back_menu.png",
                        menuTexture, menuMask)) {
            return false;
        }
    }
    if (!bgm_manager::playLoop(
            "resources/audio/bgm/gameover/gameover_01.mp3")) {
        std::cerr << "Nao foi possivel iniciar a musica de game over."
                  << std::endl;
    }
    return true;
}

bool TelaGameOver::isOpaque(const SDL_Surface* surface, int x, int y) const {
    if (!surface || x < 0 || y < 0 ||
        x >= surface->w || y >= surface->h) {
        return false;
    }
    const Uint8* row = static_cast<const Uint8*>(surface->pixels) +
                       y * surface->pitch;
    Uint32 pixel = 0;
    std::memcpy(&pixel, row + x * surface->format->BytesPerPixel,
                surface->format->BytesPerPixel);
    Uint8 red, green, blue, alpha;
    SDL_GetRGBA(pixel, surface->format, &red, &green, &blue, &alpha);
    (void)red;
    (void)green;
    (void)blue;
    return alpha > 10;
}

bool TelaGameOver::hitButton(const SDL_Surface* mask,
                             int mouseX, int mouseY) const {
    if (!mask || mouseX < 0 || mouseY < 0 ||
        mouseX >= screenW || mouseY >= screenH) {
        return false;
    }
    const int pixelX = mouseX * mask->w / screenW;
    const int pixelY = mouseY * mask->h / screenH;
    return isOpaque(mask, pixelX, pixelY);
}

void TelaGameOver::handleEvent(const SDL_Event& event) {
    if (event.type == SDL_QUIT) {
        running = false;
        return;
    }
    if (event.type != SDL_MOUSEBUTTONDOWN ||
        event.button.button != SDL_BUTTON_LEFT) {
        return;
    }
    if (hitButton(reviveMask, event.button.x, event.button.y)) {
        selectedChoice = Choice::REVIVE;
    } else if (hitButton(menuMask, event.button.x, event.button.y)) {
        selectedChoice = Choice::RETURN_TO_MENU;
    }
}

void TelaGameOver::update(float dt) {
    (void)dt;
}

void TelaGameOver::render() {
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);
    const SDL_Rect screen = {0, 0, screenW, screenH};
    if (backgroundTexture) {
        SDL_RenderCopy(renderer, backgroundTexture, nullptr, &screen);
    }
    if (reviveTexture) {
        SDL_RenderCopy(renderer, reviveTexture, nullptr, &screen);
    }
    if (menuTexture) {
        SDL_RenderCopy(renderer, menuTexture, nullptr, &screen);
    }
}

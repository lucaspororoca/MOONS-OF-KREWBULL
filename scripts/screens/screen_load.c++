#include "scripts/screens/screen_load.h"

#include "scripts/resources/resource_archive.h"

#include <algorithm>
#include <cstring>
#include <iostream>

namespace {
constexpr int ROW_TOP = 150;
constexpr int ROW_HEIGHT = 125;
constexpr int BUTTON_WIDTH = 120;
constexpr int BUTTON_HEIGHT = 100;
}

TelaLoad::TelaLoad(SDL_Renderer* renderer, int screenW, int screenH)
    : renderer(renderer),
      screenW(screenW),
      screenH(screenH),
      running(true),
      startGame(false),
      returnToIntro(false),
      backgroundTexture(nullptr),
      loadButtonTexture(nullptr),
      loadButtonMask(nullptr),
      font(nullptr),
      firstVisible(0)
{
}

TelaLoad::~TelaLoad() {
    if (backgroundTexture) SDL_DestroyTexture(backgroundTexture);
    if (loadButtonTexture) SDL_DestroyTexture(loadButtonTexture);
    if (loadButtonMask) SDL_FreeSurface(loadButtonMask);
    if (font) TTF_CloseFont(font);
}

bool TelaLoad::loadTextureFixed(const std::string& path,
                               SDL_Texture*& out, int width, int height) {
    SDL_Surface* source = resource_archive::loadResourceImage(path);
    if (!source) {
        std::cerr << "Falha ao carregar " << path << ": "
                  << IMG_GetError() << std::endl;
        return false;
    }
    SDL_Surface* scaled = SDL_CreateRGBSurfaceWithFormat(
        0, width, height, 32, SDL_PIXELFORMAT_RGBA32);
    if (!scaled) {
        std::cerr << "Falha ao criar surface para " << path << ": "
                  << SDL_GetError() << std::endl;
        SDL_FreeSurface(source);
        return false;
    }
    SDL_SetSurfaceBlendMode(source, SDL_BLENDMODE_BLEND);
    if (SDL_BlitScaled(source, nullptr, scaled, nullptr) < 0) {
        std::cerr << "Falha ao dimensionar " << path << ": "
                  << SDL_GetError() << std::endl;
        SDL_FreeSurface(scaled);
        SDL_FreeSurface(source);
        return false;
    }
    out = SDL_CreateTextureFromSurface(renderer, scaled);
    SDL_FreeSurface(scaled);
    SDL_FreeSurface(source);
    if (!out) {
        std::cerr << "Falha ao criar textura de " << path << ": "
                  << SDL_GetError() << std::endl;
        return false;
    }
    SDL_SetTextureScaleMode(out, SDL_ScaleModeNearest);
    return true;
}

bool TelaLoad::loadButton(const std::string& path) {
    SDL_Surface* source = resource_archive::loadResourceImage(path);
    if (!source) {
        std::cerr << "Falha ao carregar " << path << ": "
                  << IMG_GetError() << std::endl;
        return false;
    }
    loadButtonMask = SDL_ConvertSurfaceFormat(
        source, SDL_PIXELFORMAT_RGBA32, 0);
    SDL_FreeSurface(source);
    if (!loadButtonMask) {
        std::cerr << "Falha ao converter " << path << ": "
                  << SDL_GetError() << std::endl;
        return false;
    }
    loadButtonTexture = SDL_CreateTextureFromSurface(renderer, loadButtonMask);
    if (!loadButtonTexture) {
        std::cerr << "Falha ao criar textura de " << path << ": "
                  << SDL_GetError() << std::endl;
        return false;
    }
    SDL_SetTextureScaleMode(loadButtonTexture, SDL_ScaleModeNearest);
    return true;
}

bool TelaLoad::init() {
    if (!loadTextureFixed(
            "resources/interfaces/scenes/load/scene_load_01.png",
            backgroundTexture, screenW, screenH)) {
        statusMessage = "Background da tela de load nao encontrado.";
    }
    if (!loadButton(
            "resources/interfaces/buttons/button_load_2.png")) {
        return false;
    }
    if (TTF_WasInit() == 0 && TTF_Init() < 0) {
        std::cerr << "TTF_Init falhou: " << TTF_GetError() << std::endl;
        return false;
    }
    font = resource_archive::openResourceFont(
        "resources/fonts/impact.ttf", 25);
    if (!font) {
        std::cerr << "Falha ao carregar fonte da tela de load: "
                  << TTF_GetError() << std::endl;
        return false;
    }

    std::vector<std::filesystem::path> savePaths;
    std::string error;
    if (!player_save::listSaveFiles(savePaths, error)) {
        std::cerr << "Falha ao listar saves: " << error << std::endl;
        return false;
    }
    for (const std::filesystem::path& path : savePaths) {
        SaveEntry entry;
        entry.path = path;
        if (!player_save::readFile(path, entry.character, error)) {
            std::cerr << "Save ignorado por conter dados invalidos: "
                      << path.string() << ": " << error << std::endl;
            continue;
        }
        entries.push_back(entry);
    }
    if (entries.empty()) {
        statusMessage = "Nenhum save valido foi encontrado.";
    } else {
        statusMessage.clear();
    }
    return true;
}

int TelaLoad::visibleEntryCount() const {
    const int rowsAvailable = std::max(0, (screenH - ROW_TOP - 35) / ROW_HEIGHT);
    return std::min(rowsAvailable, 5);
}

bool TelaLoad::isPixelOpaque(int x, int y) const {
    if (!loadButtonMask || x < 0 || y < 0 ||
        x >= loadButtonMask->w || y >= loadButtonMask->h) return false;
    const Uint8* pixel = static_cast<const Uint8*>(loadButtonMask->pixels) +
        y * loadButtonMask->pitch + x * loadButtonMask->format->BytesPerPixel;
    Uint32 value = 0;
    std::memcpy(&value, pixel, loadButtonMask->format->BytesPerPixel);
    Uint8 red = 0;
    Uint8 green = 0;
    Uint8 blue = 0;
    Uint8 alpha = 0;
    SDL_GetRGBA(value, loadButtonMask->format, &red, &green, &blue, &alpha);
    return alpha > 10;
}

bool TelaLoad::isPointOverButton(int mx, int my, int y) const {
    const SDL_Rect buttonRect = {
        screenW - BUTTON_WIDTH - 85, y + (ROW_HEIGHT - BUTTON_HEIGHT) / 2,
        BUTTON_WIDTH, BUTTON_HEIGHT
    };
    if (mx < buttonRect.x || mx >= buttonRect.x + buttonRect.w ||
        my < buttonRect.y || my >= buttonRect.y + buttonRect.h) return false;
    const int pixelX = (mx - buttonRect.x) * loadButtonMask->w / buttonRect.w;
    const int pixelY = (my - buttonRect.y) * loadButtonMask->h / buttonRect.h;
    return isPixelOpaque(pixelX, pixelY);
}

void TelaLoad::handleEvent(const SDL_Event& event) {
    if (event.type == SDL_QUIT) {
        running = false;
        return;
    }
    if (event.type == SDL_KEYDOWN) {
        if (event.key.keysym.scancode == SDL_SCANCODE_ESCAPE) {
            returnToIntro = true;
        } else if (event.key.keysym.scancode == SDL_SCANCODE_DOWN) {
            if (firstVisible + static_cast<std::size_t>(visibleEntryCount()) <
                entries.size()) ++firstVisible;
        } else if (event.key.keysym.scancode == SDL_SCANCODE_UP &&
                   firstVisible > 0) {
            --firstVisible;
        }
        return;
    }
    if (event.type == SDL_MOUSEWHEEL) {
        if (event.wheel.y < 0 &&
            firstVisible + static_cast<std::size_t>(visibleEntryCount()) <
                entries.size()) {
            ++firstVisible;
        } else if (event.wheel.y > 0 && firstVisible > 0) {
            --firstVisible;
        }
        return;
    }
    if (event.type != SDL_MOUSEBUTTONDOWN ||
        event.button.button != SDL_BUTTON_LEFT) return;

    for (int i = 0; i < visibleEntryCount(); ++i) {
        const std::size_t index = firstVisible + static_cast<std::size_t>(i);
        if (index >= entries.size()) break;
        const int y = ROW_TOP + i * ROW_HEIGHT;
        if (isPointOverButton(event.button.x, event.button.y, y)) {
            selectedPath = entries[index].path;
            startGame = true;
            return;
        }
    }
}

void TelaLoad::update(float dt) {
    (void)dt;
    int mouseX = 0;
    int mouseY = 0;
    SDL_GetMouseState(&mouseX, &mouseY);
    for (SaveEntry& entry : entries) entry.hovered = false;
    for (int i = 0; i < visibleEntryCount(); ++i) {
        const std::size_t index = firstVisible + static_cast<std::size_t>(i);
        if (index >= entries.size()) break;
        entries[index].hovered =
            isPointOverButton(mouseX, mouseY, ROW_TOP + i * ROW_HEIGHT);
    }
}

void TelaLoad::renderText(const std::string& text, int x, int y,
                          SDL_Color color) {
    SDL_Surface* surface = TTF_RenderUTF8_Blended(font, text.c_str(), color);
    if (!surface) {
        std::cerr << "Falha ao renderizar texto da tela de load: "
                  << TTF_GetError() << std::endl;
        return;
    }
    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
    if (!texture) {
        std::cerr << "Falha ao criar textura de texto da tela de load: "
                  << SDL_GetError() << std::endl;
        SDL_FreeSurface(surface);
        return;
    }
    SDL_Rect destination = {x, y, surface->w, surface->h};
    SDL_RenderCopy(renderer, texture, nullptr, &destination);
    SDL_DestroyTexture(texture);
    SDL_FreeSurface(surface);
}

void TelaLoad::render() {
    SDL_SetRenderDrawColor(renderer, 18, 20, 28, 255);
    SDL_RenderClear(renderer);
    if (backgroundTexture) {
        const SDL_Rect backgroundRect = {0, 0, screenW, screenH};
        SDL_RenderCopy(renderer, backgroundTexture, nullptr, &backgroundRect);
    } else {
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 175);
        SDL_Rect panel = {45, 40, screenW - 90, screenH - 80};
        SDL_RenderFillRect(renderer, &panel);
    }

    const SDL_Color titleColor = {255, 220, 120, 255};
    const SDL_Color labelColor = {230, 230, 230, 255};
    renderText("Carregar jogo", 80, 48, titleColor);
    renderText("Personagem", 120, 105, labelColor);
    renderText("Classe", 610, 105, labelColor);
    renderText("Nivel", 900, 105, labelColor);

    if (entries.empty()) {
        const SDL_Color messageColor = {255, 210, 130, 255};
        renderText(statusMessage.empty()
                       ? "Nenhum save valido foi encontrado."
                       : statusMessage,
                   120, ROW_TOP + 25, messageColor);
    }

    for (int i = 0; i < visibleEntryCount(); ++i) {
        const std::size_t index = firstVisible + static_cast<std::size_t>(i);
        if (index >= entries.size()) break;
        const SaveEntry& entry = entries[index];
        const int rowY = ROW_TOP + i * ROW_HEIGHT;
        SDL_SetRenderDrawColor(renderer,
            entry.hovered ? 90 : 24,
            entry.hovered ? 74 : 28,
            entry.hovered ? 48 : 38,
            220);
        const SDL_Rect rowRect = {80, rowY, screenW - 160, ROW_HEIGHT - 8};
        SDL_RenderFillRect(renderer, &rowRect);
        renderText(entry.character.name, 120, rowY + 42, labelColor);
        renderText(entry.character.cls, 610, rowY + 42, labelColor);
        renderText(std::to_string(entry.character.level),
                   900, rowY + 42, labelColor);

        const SDL_Rect buttonRect = {
            screenW - BUTTON_WIDTH - 85,
            rowY + (ROW_HEIGHT - BUTTON_HEIGHT) / 2,
            BUTTON_WIDTH, BUTTON_HEIGHT
        };
        SDL_RenderCopy(renderer, loadButtonTexture, nullptr, &buttonRect);
    }

    if (entries.size() > static_cast<std::size_t>(visibleEntryCount())) {
        renderText("Use a roda do mouse ou as setas para navegar",
                   80, screenH - 42, labelColor);
    }
    if (!backgroundTexture) {
        const SDL_Color warningColor = {255, 160, 120, 255};
        renderText("Background scene_load_01.png nao encontrado",
                   80, screenH - 76, warningColor);
    }
}

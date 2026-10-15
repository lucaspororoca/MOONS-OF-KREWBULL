#include "scripts/objects/door.h"
#include "scripts/dice/dice_1d20.h"
#include "scripts/characters/player.h"
#include "scripts/resources/resource_archive.h"
#include "scripts/resources/player_save.h"
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <iostream>
#include <sstream>

static bool doorRngSeeded = false;

Door::Door()
    : textboxRollTex(nullptr), textboxSuccessTex(nullptr),
      textboxFailedTex(nullptr),
      buttonRollTex(nullptr), buttonRollHoverTex(nullptr),
      buttonCancelTex(nullptr), buttonCancelHoverTex(nullptr),
      noticeOptionsOpenTex(nullptr),
      noticeOptionsOpenW(0), noticeOptionsOpenH(0),
      noticeOptionsOpenDoor01Tex(nullptr),
      noticeOptionsOpenDoor01W(0), noticeOptionsOpenDoor01H(0),
      buttonRollMask(nullptr), buttonCancelMask(nullptr),
      font(nullptr), fontSmall(nullptr), screenW(1400), screenH(850),
      state(STATE_IDLE),
      pendingDoorRow(-1), pendingDoorCol(-1), pendingDoorId(0),
      diceResult(0), strengthModifier(0), stateTimer(0.0f),
      rollHovered(false), cancelHovered(false)
{
    for (int i = 0; i < 20; ++i) diceFaceTex[i] = nullptr;
    if (!doorRngSeeded) {
        std::srand(static_cast<unsigned>(std::time(nullptr)));
        doorRngSeeded = true;
    }
}

Door::~Door() {
    for (auto& pair : textures) {
        if (pair.second) SDL_DestroyTexture(pair.second);
    }
    textures.clear();

    if (textboxRollTex)       SDL_DestroyTexture(textboxRollTex);
    if (textboxSuccessTex)    SDL_DestroyTexture(textboxSuccessTex);
    if (textboxFailedTex)     SDL_DestroyTexture(textboxFailedTex);
    if (buttonRollTex)        SDL_DestroyTexture(buttonRollTex);
    if (buttonRollHoverTex)   SDL_DestroyTexture(buttonRollHoverTex);
    if (buttonCancelTex)      SDL_DestroyTexture(buttonCancelTex);
    if (buttonCancelHoverTex) SDL_DestroyTexture(buttonCancelHoverTex);
    if (noticeOptionsOpenTex) SDL_DestroyTexture(noticeOptionsOpenTex);
    if (noticeOptionsOpenDoor01Tex) {
        SDL_DestroyTexture(noticeOptionsOpenDoor01Tex);
    }

    if (buttonRollMask)   SDL_FreeSurface(buttonRollMask);
    if (buttonCancelMask) SDL_FreeSurface(buttonCancelMask);

    for (int i = 0; i < 20; ++i) {
        if (diceFaceTex[i]) SDL_DestroyTexture(diceFaceTex[i]);
    }

    if (fontSmall) {
        TTF_CloseFont(fontSmall);
        fontSmall = nullptr;
    }
    if (font) {
        TTF_CloseFont(font);
        font = nullptr;
    }
    TTF_Quit();
}

bool Door::loadTexture(SDL_Renderer* renderer,
                       const std::string& path,
                       SDL_Texture*& out)
{
    return loadTextureFixed(renderer, path, out, TILE_SIZE, TILE_SIZE);
}

bool Door::loadTextureFixed(SDL_Renderer* renderer,
                            const std::string& path,
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

bool Door::loadButton(SDL_Renderer* renderer,
                      const std::string& path,
                      SDL_Texture*& out,
                      SDL_Surface*& mask,
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

    out = SDL_CreateTextureFromSurface(renderer, scaled);
    if (!out) {
        std::cerr << "Falha ao criar textura de " << path
                  << ": " << SDL_GetError() << std::endl;
        SDL_FreeSurface(scaled);
        SDL_FreeSurface(surf);
        return false;
    }

    SDL_SetTextureScaleMode(out, SDL_ScaleModeNearest);

    mask = scaled;
    SDL_FreeSurface(surf);
    return true;
}

bool Door::loadTextureNative(SDL_Renderer* renderer,
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

bool Door::loadDoorsData() {
    doorsData.clear();
    if (savePath.empty()) {
        std::cerr << "Caminho do save do jogador nao foi configurado para as portas."
                  << std::endl;
        return false;
    }
    player_save::Character savedCharacter;
    std::string error;
    if (!player_save::readFile(savePath, savedCharacter, error)) {
        std::cerr << "Falha ao carregar portas do save: " << error << std::endl;
        return false;
    }
    for (const player_save::DoorState& saved : savedCharacter.doors) {
        doorsData.push_back(
            {saved.id, saved.row, saved.col, saved.status, saved.difficulty});
    }
    return true;
}

void Door::saveDoorsData() const {
    if (savePath.empty()) {
        std::cerr << "Caminho do save do jogador nao foi configurado para as portas."
                  << std::endl;
        return;
    }
    std::vector<player_save::DoorState> savedDoors;
    savedDoors.reserve(doorsData.size());
    for (const DoorData& door : doorsData) {
        savedDoors.push_back(
            {door.id, door.row, door.col, door.status, door.difficulty});
    }
    std::string error;
    if (!player_save::writeDoors(savePath, savedDoors, error)) {
        std::cerr << "Falha ao salvar portas no save: " << error << std::endl;
    }
}

Door::DoorData* Door::findDoorData(int row, int col) {
    for (auto& d : doorsData) {
        if (d.row == row && d.col == col) return &d;
    }
    return nullptr;
}

const Door::DoorData* Door::findDoorData(int row, int col) const {
    for (const auto& d : doorsData) {
        if (d.row == row && d.col == col) return &d;
    }
    return nullptr;
}

Door::DoorData& Door::ensureDoorData(int row, int col) {
    DoorData* existing = findDoorData(row, col);
    if (existing) return *existing;

    DoorData d;
    d.row        = row;
    d.col        = col;
    d.status     = "closed";
    d.difficulty = 5 + (std::rand() % 8);

    int maxId = 0;
    for (const auto& e : doorsData) {
        if (e.id > maxId) maxId = e.id;
    }
    d.id = maxId + 1;

    doorsData.push_back(d);
    return doorsData.back();
}

bool Door::load(SDL_Renderer* renderer) {
    SDL_Texture* tex1 = nullptr;
    SDL_Texture* tex2 = nullptr;
    if (!loadTexture(renderer,
                     "resources/sprites/objects/door/door_01.png",
                     tex1)) {
        return false;
    }
    if (!loadTexture(renderer,
                     "resources/sprites/objects/door/door_02.png",
                     tex2)) {
        return false;
    }
    textures[1] = tex1;
    textures[2] = tex2;

    configs[1] = { 0, 10 };
    configs[2] = { 3, 12 };

    loadTextureFixed(renderer,
        "resources/interfaces/textbox/textbox_roll_01.png",
        textboxRollTex, 1400, 850);
    loadTextureFixed(renderer,
        "resources/interfaces/notices/textbox_roll_successful.png",
        textboxSuccessTex, 1400, 850);
    loadTextureFixed(renderer,
        "resources/interfaces/notices/textbox_roll_falied.png",
        textboxFailedTex, 1400, 850);

    if (!loadButton(renderer,
                    "resources/interfaces/buttons/button_roll_idle.png",
                    buttonRollTex, buttonRollMask, 1400, 850)) {
        return false;
    }
    loadTextureFixed(renderer,
        "resources/interfaces/buttons/button_roll_hover.png",
        buttonRollHoverTex, 1400, 850);

    if (!loadButton(renderer,
                    "resources/interfaces/buttons/button_cancel_idle.png",
                    buttonCancelTex, buttonCancelMask, 1400, 850)) {
        return false;
    }
    loadTextureFixed(renderer,
        "resources/interfaces/buttons/button_cancel_hover.png",
        buttonCancelHoverTex, 1400, 850);

    for (int i = 1; i <= 20; ++i) {
        std::string path = "resources/interfaces/dice/1d20_"
                         + std::to_string(i) + ".png";
        loadTextureFixed(renderer, path, diceFaceTex[i - 1], 1400, 850);
    }

    loadTextureNative(renderer,
        "resources/interfaces/notices/notice_options_open.png",
        noticeOptionsOpenTex,
        noticeOptionsOpenW,
        noticeOptionsOpenH);
    if (!loadTextureNative(renderer,
            "resources/interfaces/notices/notice_options_open_2.png",
            noticeOptionsOpenDoor01Tex,
            noticeOptionsOpenDoor01W,
            noticeOptionsOpenDoor01H)) {
        return false;
    }

    textboxRect      = { 0, 0, 1400, 850 };
    buttonRollRect   = { 0, 0, 1400, 850 };
    buttonCancelRect = { 0, 0, 1400, 850 };
    diceRect         = { 0, 0, 1400, 850 };

    if (TTF_Init() < 0) {
        std::cerr << "TTF_Init falhou: " << TTF_GetError() << std::endl;
        return false;
    }

    font = resource_archive::openResourceFont("resources/fonts/impact.ttf", 28);
    if (!font) {
        std::cerr << "Falha ao carregar fonte: " << TTF_GetError() << std::endl;
        return false;
    }

    fontSmall = resource_archive::openResourceFont("resources/fonts/impact.ttf", 45);
    if (!fontSmall) {
        std::cerr << "Falha ao carregar fonte 12: " << TTF_GetError() << std::endl;
        return false;
    }

    if (!loadDoorsData()) return false;

    return true;
}

SDL_Texture* Door::getTexture(int id) const {
    auto it = textures.find(id);
    if (it != textures.end()) return it->second;
    return nullptr;
}

bool Door::isDoor(char c) const {
    if (c < 'a' || c > 'z') return false;
    int id = c - 'a' + 1;
    return textures.find(id) != textures.end();
}

int Door::charToId(char c) const {
    if (!isDoor(c)) return -1;
    return c - 'a' + 1;
}

bool Door::findActiveDoor(const std::vector<std::string>& tiles,
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
            if (isDoor(tiles[ty][tx])) {
                outRow = ty;
                outCol = tx;
                return true;
            }
        }
    }
    return false;
}

bool Door::startRoll(int row, int col, int doorId,
                     std::vector<std::string>& tiles, int strength) {
    pendingDoorRow = row;
    pendingDoorCol = col;
    pendingDoorId  = doorId;
    diceResult = 0;
    strengthModifier = static_cast<int>(
        std::floor((strength - 10) / 2.0));
    stateTimer = 0.0f;
    rollHovered = false;
    cancelHovered = false;

    DoorData& doorData = ensureDoorData(row, col);
    if (doorId == 1) {
        if (row < 0 || col < 0 || row >= static_cast<int>(tiles.size()) ||
            col >= static_cast<int>(tiles[row].size()) ||
            !isDoor(tiles[row][col])) {
            std::cerr << "Nao foi possivel abrir a door_01 na posicao informada."
                      << std::endl;
            state = STATE_IDLE;
            return false;
        }
        tiles[row][col] = '1';
        doorData.status = "open";
        saveDoorsData();
        state = STATE_IDLE;
        pendingDoorRow = -1;
        pendingDoorCol = -1;
        pendingDoorId = 0;
        return true;
    }

    state = STATE_ROLL_UI;
    saveDoorsData();
    return false;
}

bool Door::isPixelOpaque(SDL_Surface* surf, int x, int y) const {
    if (!surf) return false;
    if (x < 0 || y < 0 || x >= surf->w || y >= surf->h) return false;

    Uint32* pixels = static_cast<Uint32*>(surf->pixels);
    Uint32 pixel = pixels[y * surf->w + x];
    Uint8 a = static_cast<Uint8>((pixel & surf->format->Amask) >> surf->format->Ashift);
    return a > 10;
}

bool Door::isPointOverButton(int mx, int my, const SDL_Rect& rect,
                             SDL_Surface* mask) const
{
    if (mx < rect.x || mx >= rect.x + rect.w) return false;
    if (my < rect.y || my >= rect.y + rect.h) return false;

    if (!mask) return true;

    int px = mx - rect.x;
    int py = my - rect.y;

    if (mask->w != rect.w || mask->h != rect.h) {
        px = px * mask->w / rect.w;
        py = py * mask->h / rect.h;
    }

    return isPixelOpaque(mask, px, py);
}

void Door::updateHover() {
    if (state != STATE_ROLL_UI) {
        rollHovered = false;
        cancelHovered = false;
        return;
    }

    int mx, my;
    SDL_GetMouseState(&mx, &my);

    rollHovered   = isPointOverButton(mx, my, buttonRollRect,   buttonRollMask);
    cancelHovered = isPointOverButton(mx, my, buttonCancelRect, buttonCancelMask);
}

void Door::handleEvent(const SDL_Event& event) {
    if (state != STATE_ROLL_UI) return;
    if (event.type != SDL_MOUSEBUTTONDOWN) return;
    if (event.button.button != SDL_BUTTON_LEFT) return;

    int mx = event.button.x;
    int my = event.button.y;

    if (isPointOverButton(mx, my, buttonRollRect, buttonRollMask)) {
        diceResult = Dice1d20::roll();
        stateTimer = 0.0f;
        state = STATE_ROLLING;
        rollHovered = false;
        cancelHovered = false;
        return;
    }

    if (isPointOverButton(mx, my, buttonCancelRect, buttonCancelMask)) {
        stateTimer = 0.0f;
        state = STATE_IDLE;
        rollHovered = false;
        cancelHovered = false;
        return;
    }
}

bool Door::rollSuccess() const {
    const DoorData* d = findDoorData(pendingDoorRow, pendingDoorCol);
    if (!d) return false;

    const int total = diceResult + strengthModifier;
    return total >= d->difficulty;
}

void Door::renderRollModifier(SDL_Renderer* renderer) const {
    if (!font || pendingDoorId != 2 ||
        state == STATE_IDLE || pendingDoorRow < 0 || pendingDoorCol < 0) {
        return;
    }

    std::string text = "(";
    if (strengthModifier >= 0) text += "+";
    text += std::to_string(strengthModifier) + ")";
    if (diceResult >= 1 && diceResult <= 20) {
        text += " = " + std::to_string(diceResult + strengthModifier);
    }

    const SDL_Color color = {0xFF, 0xC4, 0x01, 255};
    SDL_Surface* surface = TTF_RenderUTF8_Blended(font, text.c_str(), color);
    if (!surface) {
        std::cerr << "Falha ao renderizar modificador de Forca da porta: "
                  << TTF_GetError() << std::endl;
        return;
    }

    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
    if (!texture) {
        std::cerr << "Falha ao criar textura do modificador de Forca: "
                  << SDL_GetError() << std::endl;
        SDL_FreeSurface(surface);
        return;
    }

    const SDL_Rect destination = {
        1100 - surface->w,
        330,
        surface->w,
        surface->h
    };
    SDL_RenderCopy(renderer, texture, nullptr, &destination);
    SDL_DestroyTexture(texture);
    SDL_FreeSurface(surface);
}

bool Door::applySavedStates(std::vector<std::string>& tiles) const {
    bool changed = false;
    for (const DoorData& saved : doorsData) {
        if (saved.status != "open" || saved.row < 0 || saved.col < 0 ||
            saved.row >= static_cast<int>(tiles.size()) ||
            saved.col >= static_cast<int>(tiles[saved.row].size())) {
            continue;
        }
        if (isDoor(tiles[saved.row][saved.col])) {
            tiles[saved.row][saved.col] = '1';
            changed = true;
        }
    }
    return changed;
}

bool Door::update(float dt, std::vector<std::string>& tiles) {
    if (state == STATE_IDLE) return false;

    stateTimer += dt;

    if (state == STATE_ROLL_UI) {
        updateHover();
        return false;
    }

    if (state == STATE_ROLLING && stateTimer >= 1.5f) {
        if (rollSuccess()) {
            stateTimer = 0.0f;
            state = STATE_SUCCESS;
        } else {
            stateTimer = 0.0f;
            state = STATE_FAILED;
        }
        return false;
    }

    if (state == STATE_FAILED && stateTimer >= 1.5f) {
        pendingDoorRow = -1;
        pendingDoorCol = -1;
        pendingDoorId = 0;
        diceResult = 0;
        strengthModifier = 0;
        stateTimer = 0.0f;
        state = STATE_IDLE;
        return false;
    }

    if (state == STATE_SUCCESS && stateTimer >= 1.5f) {
        bool opened = false;
        if (pendingDoorRow >= 0 && pendingDoorCol >= 0 &&
            pendingDoorRow < static_cast<int>(tiles.size()) &&
            pendingDoorCol < static_cast<int>(tiles[pendingDoorRow].size())) {
            if (isDoor(tiles[pendingDoorRow][pendingDoorCol])) {
                tiles[pendingDoorRow][pendingDoorCol] = '1';
                opened = true;

                DoorData* d = findDoorData(pendingDoorRow, pendingDoorCol);
                if (!d) {
                    d = &ensureDoorData(pendingDoorRow, pendingDoorCol);
                }
                d->status = "open";
                saveDoorsData();
            }
        }
        pendingDoorRow = -1;
        pendingDoorCol = -1;
        pendingDoorId  = 0;
        diceResult = 0;
        strengthModifier = 0;
        stateTimer = 0.0f;
        state = STATE_IDLE;
        return opened;
    }
    return false;
}

void Door::renderPrompt(SDL_Renderer* renderer, int camX, int camY,
                        const Player& player, int doorId) const
{
    SDL_Texture* promptTexture = doorId == 1
        ? noticeOptionsOpenDoor01Tex
        : noticeOptionsOpenTex;
    if (!promptTexture) return;

    const int GAP = 10;
    const int PLAYER_SPRITE = Player::SPRITE_SIZE;
    const int promptWidth = doorId == 1
        ? noticeOptionsOpenDoor01W
        : noticeOptionsOpenW;
    const int promptHeight = doorId == 1
        ? noticeOptionsOpenDoor01H
        : noticeOptionsOpenH;

    int playerScreenX = static_cast<int>(player.getX()) - camX;
    int playerScreenY = static_cast<int>(player.getY()) - camY;

    SDL_Rect dst;
    dst.w = promptWidth;
    dst.h = promptHeight;
    dst.x = playerScreenX + PLAYER_SPRITE / 2 + GAP;
    dst.y = playerScreenY - promptHeight / 2;

    SDL_RenderCopy(renderer, promptTexture, nullptr, &dst);
}

void Door::renderDifficulty(SDL_Renderer* renderer) const {
    if (!fontSmall) return;
    if (pendingDoorRow < 0 || pendingDoorCol < 0) return;

    const DoorData* d = findDoorData(pendingDoorRow, pendingDoorCol);
    if (!d) return;
    if (d->status != "closed") return;

    std::string text = std::to_string(d->difficulty);
    SDL_Color color = { 0x3e, 0x55, 0xff, 255 }; // #3e55ff
    SDL_Surface* surf = TTF_RenderText_Solid(fontSmall, text.c_str(), color);
    if (!surf) return;

    SDL_Texture* tex = SDL_CreateTextureFromSurface(renderer, surf);
    if (tex) {
        SDL_Rect dst;
        dst.w = surf->w;
        dst.h = surf->h;
        dst.x = 1055;
        dst.y = 260;
        SDL_RenderCopy(renderer, tex, nullptr, &dst);
        SDL_DestroyTexture(tex);
    }
    SDL_FreeSurface(surf);
}

void Door::renderUI(SDL_Renderer* renderer) const {
    if (state == STATE_IDLE) return;

    if (state == STATE_ROLL_UI) {
        if (textboxRollTex) SDL_RenderCopy(renderer, textboxRollTex, nullptr, &textboxRect);

        SDL_Texture* rollTex = rollHovered && buttonRollHoverTex
                             ? buttonRollHoverTex
                             : buttonRollTex;
        if (rollTex) SDL_RenderCopy(renderer, rollTex, nullptr, &buttonRollRect);

        SDL_Texture* cancelTex = cancelHovered && buttonCancelHoverTex
                               ? buttonCancelHoverTex
                               : buttonCancelTex;
        if (cancelTex) SDL_RenderCopy(renderer, cancelTex, nullptr, &buttonCancelRect);

        renderDifficulty(renderer);
        renderRollModifier(renderer);
        return;
    }

    if (state == STATE_ROLLING) {
        if (textboxRollTex) SDL_RenderCopy(renderer, textboxRollTex, nullptr, &textboxRect);
        if (diceResult >= 1 && diceResult <= 20 && diceFaceTex[diceResult - 1]) {
            SDL_RenderCopy(renderer, diceFaceTex[diceResult - 1], nullptr, &diceRect);
        }
        renderDifficulty(renderer);
        renderRollModifier(renderer);
        return;
    }

    if (state == STATE_SUCCESS) {
        if (textboxRollTex) SDL_RenderCopy(renderer, textboxRollTex, nullptr, &textboxRect);
        if (diceResult >= 1 && diceResult <= 20 && diceFaceTex[diceResult - 1]) {
            SDL_RenderCopy(renderer, diceFaceTex[diceResult - 1], nullptr, &diceRect);
        }
        renderDifficulty(renderer);
        renderRollModifier(renderer);
        if (textboxSuccessTex) {
            SDL_RenderCopy(renderer, textboxSuccessTex, nullptr, &textboxRect);
        }
        return;
    }

    if (state == STATE_FAILED) {
        if (textboxRollTex) SDL_RenderCopy(renderer, textboxRollTex, nullptr, &textboxRect);
        if (diceResult >= 1 && diceResult <= 20 && diceFaceTex[diceResult - 1]) {
            SDL_RenderCopy(renderer, diceFaceTex[diceResult - 1], nullptr, &diceRect);
        }
        renderDifficulty(renderer);
        renderRollModifier(renderer);
        if (textboxFailedTex) {
            SDL_RenderCopy(renderer, textboxFailedTex, nullptr, &textboxRect);
        }
        return;
    }
}
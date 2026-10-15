#include "scripts/hud/bag.h"
#include "scripts/resources/player_save.h"
#include "scripts/resources/resource_archive.h"
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <sstream>

Bag::Bag()
    : bagIdleTex(nullptr), bagHoverTex(nullptr), bagMask(nullptr),
      bagHovered(false),
      panelTex(nullptr),
      returnIdleTex(nullptr), returnHoverTex(nullptr), returnMask(nullptr),
      returnHovered(false),
      dropButtonTex(nullptr), dropMask(nullptr),
      dropBtnW(0), dropBtnH(0),
      useButtonTex(nullptr), useMask(nullptr),
      useBtnW(0), useBtnH(0),
      pendingDropIdx(-1),
      pendingUseIdx(-1),
      font(nullptr), open(false)
{
    bagRect    = { 0, 0, 0, 0 };
    panelRect  = { 0, 0, 0, 0 };
    returnRect = { 0, 0, 0, 0 };
}

Bag::~Bag() {
    if (bagIdleTex)     SDL_DestroyTexture(bagIdleTex);
    if (bagHoverTex)    SDL_DestroyTexture(bagHoverTex);
    if (bagMask)        SDL_FreeSurface(bagMask);

    if (panelTex)       SDL_DestroyTexture(panelTex);

    if (returnIdleTex)  SDL_DestroyTexture(returnIdleTex);
    if (returnHoverTex) SDL_DestroyTexture(returnHoverTex);
    if (returnMask)     SDL_FreeSurface(returnMask);

    if (dropButtonTex)  SDL_DestroyTexture(dropButtonTex);
    if (dropMask)       SDL_FreeSurface(dropMask);

    if (useButtonTex)   SDL_DestroyTexture(useButtonTex);
    if (useMask)        SDL_FreeSurface(useMask);

    if (font) {
        TTF_CloseFont(font);
        font = nullptr;
    }
}

bool Bag::load() {
    items.clear();
    if (savePath.empty()) {
        std::cerr << "Caminho do save do jogador nao foi configurado para a bag."
                  << std::endl;
        return false;
    }
    player_save::Character savedCharacter;
    std::string error;
    if (!player_save::readFile(savePath, savedCharacter, error)) {
        std::cerr << "Falha ao carregar a bag do save: " << error << std::endl;
        return false;
    }
    for (const player_save::BagItem& saved : savedCharacter.bag) {
        items.push_back({saved.name, saved.quantity, saved.weight, saved.state});
    }
    return true;
}

void Bag::save() const {
    if (savePath.empty()) {
        std::cerr << "Caminho do save do jogador nao foi configurado para a bag."
                  << std::endl;
        return;
    }
    std::vector<player_save::BagItem> savedItems;
    savedItems.reserve(items.size());
    for (const Item& item : items) {
        savedItems.push_back({item.name, item.quantity, item.weight, item.state});
    }
    std::string error;
    if (!player_save::writeBag(savePath, savedItems, error)) {
        std::cerr << "Falha ao salvar a bag no save: " << error << std::endl;
    }
}

void Bag::addItem(const std::string& name, float weight, int quantity,
                  const std::string& state)
{
    for (auto& it : items) {
        if (it.name == name) {
            it.quantity += quantity;
            if (!state.empty()) it.state = state;
            save();
            return;
        }
    }

    Item newItem;
    newItem.name     = name;
    newItem.quantity = quantity;
    newItem.weight   = weight;
    newItem.state    = state;
    items.push_back(newItem);
    save();
}

void Bag::removeItem(int index, int quantity) {
    if (index < 0 || index >= static_cast<int>(items.size())) return;

    items[index].quantity -= quantity;
    if (items[index].quantity <= 0) {
        items.erase(items.begin() + index);
    }
    save();
}

void Bag::setItemStateByName(const std::string& name, const std::string& state) {
    for (auto& it : items) {
        if (it.name == name) {
            it.state = state;
            save();
            return;
        }
    }
}

bool Bag::consumeDrop(std::string& outName) {
    if (pendingDropIdx < 0) return false;
    outName = pendingDropItem;
    pendingDropIdx = -1;
    pendingDropItem.clear();
    return true;
}

bool Bag::consumeUse(std::string& outName) {
    if (pendingUseIdx < 0) return false;
    outName = pendingUseItem;
    pendingUseIdx = -1;
    pendingUseItem.clear();
    return true;
}

bool Bag::canUse(const Item& it) const {
    return (it.name == "Potion HP minor" && it.state != "vazio");
}

bool Bag::loadTexture(SDL_Renderer* renderer,
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

bool Bag::loadButtonWithMask(SDL_Renderer* renderer,
                             const std::string& path,
                             SDL_Texture*& out,
                             SDL_Surface*& mask,
                             int& outW, int& outH)
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

    outW = converted->w;
    outH = converted->h;
    mask = converted;
    return true;
}

bool Bag::loadUI(SDL_Renderer* renderer) {
    if (TTF_WasInit() == 0) {
        if (TTF_Init() < 0) {
            std::cerr << "TTF_Init falhou: " << TTF_GetError() << std::endl;
            return false;
        }
    }

    const int SCREEN_W = 1400;
    const int SCREEN_H = 850;

    int w = 0, h = 0;

    if (!loadButtonWithMask(renderer,
                            "resources/interfaces/buttons/button_bag_idle.png",
                            bagIdleTex, bagMask, w, h)) {
        return false;
    }
    bagRect.w = w;
    bagRect.h = h;
    bagRect.x = SCREEN_W - w - 20;
    bagRect.y = 20;

    if (!loadTexture(renderer,
                     "resources/interfaces/buttons/button_bag_hover.png",
                     bagHoverTex, w, h)) {
        return false;
    }

    if (!loadTexture(renderer,
                     "resources/interfaces/textbox/textbox_inventory.png",
                     panelTex, w, h)) {
        return false;
    }
    panelRect.w = w;
    panelRect.h = h;
    panelRect.x = (SCREEN_W - w) / 2;
    panelRect.y = (SCREEN_H - h) / 2;

    if (!loadButtonWithMask(renderer,
                            "resources/interfaces/buttons/button_return_idle.png",
                            returnIdleTex, returnMask, w, h)) {
        return false;
    }
    returnRect.w = w;
    returnRect.h = h;
    returnRect.x = panelRect.x + panelRect.w - w - 40;
    returnRect.y = panelRect.y + panelRect.h - h - 40;

    if (!loadTexture(renderer,
                     "resources/interfaces/buttons/button_return_hover.png",
                     returnHoverTex, w, h)) {
        return false;
    }

    if (!loadButtonWithMask(renderer,
                            "resources/interfaces/buttons/button_drop.png",
                            dropButtonTex, dropMask, dropBtnW, dropBtnH)) {
        return false;
    }

    if (!loadButtonWithMask(renderer,
                            "resources/interfaces/buttons/button_use.png",
                            useButtonTex, useMask, useBtnW, useBtnH)) {
        return false;
    }

    font = resource_archive::openResourceFont("resources/fonts/impact.ttf", 20);
    if (!font) {
        std::cerr << "Falha ao carregar fonte: " << TTF_GetError() << std::endl;
        return false;
    }

    return true;
}

bool Bag::isPixelOpaque(SDL_Surface* surf, int x, int y) const {
    if (!surf) return false;
    if (x < 0 || y < 0 || x >= surf->w || y >= surf->h) return false;

    Uint32* pixels = static_cast<Uint32*>(surf->pixels);
    Uint32 pixel = pixels[y * surf->w + x];
    Uint8 a = static_cast<Uint8>(
        (pixel & surf->format->Amask) >> surf->format->Ashift);
    return a > 10;
}

bool Bag::isPointOverRect(int mx, int my, const SDL_Rect& rect,
                          SDL_Surface* mask) const
{
    if (mx < rect.x || mx >= rect.x + rect.w) return false;
    if (my < rect.y || my >= rect.y + rect.h) return false;

    if (!mask) return true;

    int px = mx - rect.x;
    int py = my - rect.y;
    return isPixelOpaque(mask, px, py);
}

void Bag::computeDropButtonRects() {
    dropButtonRects.clear();

    const int startY = panelRect.y + 160;
    const int lineH  = 34;

    int y = startY + lineH;

    for (size_t i = 0; i < items.size(); ++i) {
        SDL_Rect r;
        r.w = dropBtnW;
        r.h = dropBtnH;
        r.x = panelRect.x + panelRect.w - useBtnW - dropBtnW - 80;
        r.y = y + (lineH - dropBtnH) / 2;
        dropButtonRects.push_back(r);
        y += lineH;
    }
}

void Bag::computeUseButtonRects() {
    useButtonRects.clear();

    const int startY = panelRect.y + 160;
    const int lineH  = 34;

    int y = startY + lineH;

    for (size_t i = 0; i < items.size(); ++i) {
        SDL_Rect r;
        r.w = useBtnW;
        r.h = useBtnH;
        r.x = panelRect.x + panelRect.w - useBtnW - 60;
        r.y = y + (lineH - useBtnH) / 2;
        useButtonRects.push_back(r);
        y += lineH;
    }
}

void Bag::handleEvent(const SDL_Event& event) {
    if (event.type == SDL_MOUSEBUTTONDOWN &&
        event.button.button == SDL_BUTTON_LEFT) {
        int mx = event.button.x;
        int my = event.button.y;

        if (isPointOverRect(mx, my, bagRect, bagMask)) {
            open = !open;
            bagHovered = false;
            returnHovered = false;
            return;
        }

        if (open) {
            if (isPointOverRect(mx, my, returnRect, returnMask)) {
                open = false;
                returnHovered = false;
                return;
            }

            computeDropButtonRects();
            computeUseButtonRects();

            for (size_t i = 0; i < items.size(); ++i) {
                if (canUse(items[i]) && i < useButtonRects.size()) {
                    if (isPointOverRect(mx, my, useButtonRects[i], useMask)) {
                        pendingUseItem = items[i].name;
                        pendingUseIdx  = static_cast<int>(i);
                        return;
                    }
                }
            }

            for (size_t i = 0; i < items.size(); ++i) {
                if (i < dropButtonRects.size()) {
                    if (isPointOverRect(mx, my, dropButtonRects[i], dropMask)) {
                        pendingDropItem = items[i].name;
                        pendingDropIdx  = static_cast<int>(i);
                        removeItem(static_cast<int>(i), 1);
                        return;
                    }
                }
            }
        }
    }
}

void Bag::updateUI(float dt) {
    (void)dt;

    int mx, my;
    SDL_GetMouseState(&mx, &my);

    bagHovered = isPointOverRect(mx, my, bagRect, bagMask);

    if (open) {
        returnHovered = isPointOverRect(mx, my, returnRect, returnMask);
        computeDropButtonRects();
        computeUseButtonRects();
    } else {
        returnHovered = false;
    }
}

void Bag::renderItems(SDL_Renderer* renderer) {
    if (!font) return;

    const int startX = panelRect.x + 80;
    const int startY = panelRect.y + 160;
    const int lineH  = 34;

    const int colQtyX    = panelRect.x + 520;
    const int colWeightX = panelRect.x + 650;

    SDL_Color headerColor = { 220, 220, 220, 255 };
    SDL_Color textColor   = { 255, 255, 255, 255 };
    SDL_Color emptyColor  = { 160, 160, 160, 255 };

    auto drawText = [&](const std::string& s, SDL_Color c, int x, int y) {
        if (s.empty()) return;
        SDL_Surface* surf = TTF_RenderText_Blended(font, s.c_str(), c);
        if (!surf) return;
        SDL_Texture* tex = SDL_CreateTextureFromSurface(renderer, surf);
        if (tex) {
            SDL_Rect d = { x, y, surf->w, surf->h };
            SDL_RenderCopy(renderer, tex, nullptr, &d);
            SDL_DestroyTexture(tex);
        }
        SDL_FreeSurface(surf);
    };

    drawText("ITEM",   headerColor, startX,     startY);
    drawText("QTY",    headerColor, colQtyX,    startY);
    drawText("WEIGHT", headerColor, colWeightX, startY);

    int y = startY + lineH;

    for (size_t i = 0; i < items.size(); ++i) {
        const Item& it = items[i];

        std::string displayName = it.name;
        if (!it.state.empty()) {
            displayName += " (" + it.state + ")";
        }

        std::string qtyStr = std::to_string(it.quantity);

        std::ostringstream wss;
        wss << std::fixed << std::setprecision(2)
            << (it.weight * it.quantity);

        SDL_Color nameColor = (it.state == "vazio") ? emptyColor : textColor;

        drawText(displayName, nameColor, startX,     y);
        drawText(qtyStr,      textColor, colQtyX,    y);
        drawText(wss.str(),   textColor, colWeightX, y);

        if (i < dropButtonRects.size() && dropButtonTex) {
            SDL_RenderCopy(renderer, dropButtonTex, nullptr,
                           &dropButtonRects[i]);
        }

        if (canUse(it) && i < useButtonRects.size() && useButtonTex) {
            SDL_RenderCopy(renderer, useButtonTex, nullptr,
                           &useButtonRects[i]);
        }

        y += lineH;
    }
}

void Bag::renderUI(SDL_Renderer* renderer) {
    if (open) {
        if (panelTex) {
            SDL_RenderCopy(renderer, panelTex, nullptr, &panelRect);
        }

        computeDropButtonRects();
        computeUseButtonRects();
        renderItems(renderer);

        SDL_Texture* retTex = (returnHovered && returnHoverTex)
                            ? returnHoverTex
                            : returnIdleTex;
        if (retTex) {
            SDL_RenderCopy(renderer, retTex, nullptr, &returnRect);
        }
    }

    SDL_Texture* bagTex = (bagHovered && bagHoverTex)
                        ? bagHoverTex
                        : bagIdleTex;
    if (bagTex) {
        SDL_RenderCopy(renderer, bagTex, nullptr, &bagRect);
    }
}
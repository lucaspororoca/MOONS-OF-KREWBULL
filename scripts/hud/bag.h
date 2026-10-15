#ifndef BAG_H
#define BAG_H

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_ttf.h>
#include <filesystem>
#include <string>
#include <vector>

class Bag {
public:
    struct Item {
        std::string name;
        int         quantity;
        float       weight;
        std::string state;
    };

    Bag();
    ~Bag();

    bool load();
    void save() const;
    void setSavePath(const std::filesystem::path& path) { savePath = path; }

    void addItem(const std::string& name, float weight, int quantity = 1,
                 const std::string& state = "");
    void removeItem(int index, int quantity = 1);
    void setItemStateByName(const std::string& name, const std::string& state);

    const std::vector<Item>& getItems() const { return items; }

    bool loadUI(SDL_Renderer* renderer);
    void handleEvent(const SDL_Event& event);
    void updateUI(float dt);
    void renderUI(SDL_Renderer* renderer);

    bool isOpen() const { return open; }

    bool consumeDrop(std::string& outName);
    bool consumeUse(std::string& outName);

private:
    bool loadTexture(SDL_Renderer* renderer,
                     const std::string& path,
                     SDL_Texture*& out,
                     int& outW, int& outH);
    bool loadButtonWithMask(SDL_Renderer* renderer,
                            const std::string& path,
                            SDL_Texture*& out,
                            SDL_Surface*& mask,
                            int& outW, int& outH);

    bool isPixelOpaque(SDL_Surface* surf, int x, int y) const;
    bool isPointOverRect(int mx, int my, const SDL_Rect& rect,
                         SDL_Surface* mask) const;

    bool canUse(const Item& it) const;

    void computeDropButtonRects();
    void computeUseButtonRects();
    void renderItems(SDL_Renderer* renderer);

    std::vector<Item> items;
    std::filesystem::path savePath;

    SDL_Texture* bagIdleTex;
    SDL_Texture* bagHoverTex;
    SDL_Surface* bagMask;
    SDL_Rect     bagRect;
    bool         bagHovered;

    SDL_Texture* panelTex;
    SDL_Rect     panelRect;

    SDL_Texture* returnIdleTex;
    SDL_Texture* returnHoverTex;
    SDL_Surface* returnMask;
    SDL_Rect     returnRect;
    bool         returnHovered;

    SDL_Texture*  dropButtonTex;
    SDL_Surface*  dropMask;
    int           dropBtnW;
    int           dropBtnH;
    std::vector<SDL_Rect> dropButtonRects;

    SDL_Texture*  useButtonTex;
    SDL_Surface*  useMask;
    int           useBtnW;
    int           useBtnH;
    std::vector<SDL_Rect> useButtonRects;

    int         pendingDropIdx;
    std::string pendingDropItem;

    int         pendingUseIdx;
    std::string pendingUseItem;

    TTF_Font* font;
    bool      open;
};

#endif
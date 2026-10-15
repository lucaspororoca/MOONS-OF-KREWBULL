#ifndef DOOR_H
#define DOOR_H

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_ttf.h>
#include <filesystem>
#include <string>
#include <unordered_map>
#include <vector>

class Player;

class Door {
public:
    static const int TILE_SIZE = 176;

    enum RollState {
        STATE_IDLE,
        STATE_ROLL_UI,
        STATE_ROLLING,
        STATE_SUCCESS,
        STATE_FAILED
    };

    Door();
    ~Door();

    bool load(SDL_Renderer* renderer);
    void setSavePath(const std::filesystem::path& path) { savePath = path; }
    SDL_Texture* getTexture(int id) const;

    bool isDoor(char c) const;
    int  charToId(char c) const;

    bool findActiveDoor(const std::vector<std::string>& tiles,
                        int rows, int cols,
                        float px, float py, int pw, int ph,
                        int& outRow, int& outCol) const;

    bool startRoll(int row, int col, int doorId,
                   std::vector<std::string>& tiles, int strength);
    bool isUIActive() const { return state != STATE_IDLE; }

    void handleEvent(const SDL_Event& event);
    bool update(float dt, std::vector<std::string>& tiles);
    bool applySavedStates(std::vector<std::string>& tiles) const;

    void renderPrompt(SDL_Renderer* renderer, int camX, int camY,
                      const Player& player, int doorId) const;
    void renderUI(SDL_Renderer* renderer) const;

private:
    struct DoorConfig {
        int attributeIndex;
        int dc;
    };

    struct DoorData {
        int id;
        int row;
        int col;
        std::string status;
        int difficulty;
    };

    bool loadTexture(SDL_Renderer* renderer,
                     const std::string& path,
                     SDL_Texture*& out);
    bool loadTextureFixed(SDL_Renderer* renderer,
                          const std::string& path,
                          SDL_Texture*& out,
                          int w, int h);
    bool loadButton(SDL_Renderer* renderer,
                    const std::string& path,
                    SDL_Texture*& out,
                    SDL_Surface*& mask,
                    int w, int h);
    bool loadTextureNative(SDL_Renderer* renderer,
                           const std::string& path,
                           SDL_Texture*& out,
                           int& outW, int& outH);

    bool rollSuccess() const;

    bool isPixelOpaque(SDL_Surface* surf, int x, int y) const;
    bool isPointOverButton(int mx, int my, const SDL_Rect& rect,
                           SDL_Surface* mask) const;
    void updateHover();

    bool loadDoorsData();
    void saveDoorsData() const;
    DoorData*       findDoorData(int row, int col);
    const DoorData* findDoorData(int row, int col) const;
    DoorData&       ensureDoorData(int row, int col);

    void renderDifficulty(SDL_Renderer* renderer) const;
    void renderRollModifier(SDL_Renderer* renderer) const;

    std::unordered_map<int, SDL_Texture*> textures;
    std::unordered_map<int, DoorConfig> configs;

    SDL_Texture* textboxRollTex;
    SDL_Texture* textboxSuccessTex;
    SDL_Texture* textboxFailedTex;
    SDL_Texture* buttonRollTex;
    SDL_Texture* buttonRollHoverTex;
    SDL_Texture* buttonCancelTex;
    SDL_Texture* buttonCancelHoverTex;
    SDL_Texture* diceFaceTex[20];

    SDL_Texture* noticeOptionsOpenTex;
    int noticeOptionsOpenW;
    int noticeOptionsOpenH;
    SDL_Texture* noticeOptionsOpenDoor01Tex;
    int noticeOptionsOpenDoor01W;
    int noticeOptionsOpenDoor01H;

    SDL_Surface* buttonRollMask;
    SDL_Surface* buttonCancelMask;

    TTF_Font* font;
    TTF_Font* fontSmall;

    int screenW;
    int screenH;

    RollState state;
    int pendingDoorRow;
    int pendingDoorCol;
    int pendingDoorId;
    int diceResult;
    int strengthModifier;
    float stateTimer;

    bool rollHovered;
    bool cancelHovered;

    SDL_Rect textboxRect;
    SDL_Rect buttonRollRect;
    SDL_Rect buttonCancelRect;
    SDL_Rect diceRect;

    std::vector<DoorData> doorsData;
    std::filesystem::path savePath;
};

#endif
#ifndef CAVES_OF_CHAOS_H
#define CAVES_OF_CHAOS_H

#include <SDL2/SDL.h>
#include <SDL2/SDL_mixer.h>
#include <memory>
#include <filesystem>
#include <string>
#include <utility>
#include <vector>

#include "scripts/backgrounds/floor.h"
#include "scripts/backgrounds/wall.h"
#include "scripts/objects/door.h"
#include "scripts/objects/table_empty.h"
#include "scripts/objects/table_paper.h"
#include "scripts/objects/chair.h"
#include "scripts/objects/boockcase.h"
#include "scripts/characters/combat_enemy.h"
#include "scripts/hud/bag.h"
#include "scripts/itens/potion_hp_minor.h"

class Player;
class Ogro;
class Rat;

class CavesOfChaos {
public:
    static const int TILE_SIZE = Floor::TILE_SIZE;

    CavesOfChaos();
    ~CavesOfChaos();

    bool load(SDL_Renderer* renderer);
    void setPlayerSavePath(const std::filesystem::path& path) {
        playerSavePath = path;
        bag.setSavePath(path);
        door.setSavePath(path);
    }
    void saveWorldState(const Player& player) const;

    void update(float dt, float px, float py, int pw, int ph, const Player& player);
    void handleEvent(const SDL_Event& event, Player& player);

    void render(SDL_Renderer* renderer, int camX, int camY, Player* player = nullptr);
    void renderOverlay(SDL_Renderer* renderer);

    bool collides(float px, float py, int pw, int ph) const;
    bool collidesWithEnemies(float px, float py, int pw, int ph) const;
    int countEnemiesTouching(float px, float py, int pw, int ph) const;
    std::vector<CombatEnemy*> enemiesTouching(
        float px, float py, int pw, int ph) const;
    void removeDefeatedEnemies();

    int getPixelWidth()  const { return cols * TILE_SIZE; }
    int getPixelHeight() const { return rows * TILE_SIZE; }

private:
    struct DroppedItem {
        std::string  name;
        float        weight;
        float        x;
        float        y;
        SDL_Texture* iconTex;
        int          iconW;
        int          iconH;
    };

    void loadTilesFromJson();

    void updateEnemies(float dt, const Player& player);
    void spawnOgro();
    void spawnOgro(float x, float y);
    void spawnRat();
    void spawnRat(float x, float y);
    void spawnInitialEnemies();

    void spawnDroppedItem(const std::string& name, float weight,
                          float x, float y,
                          SDL_Texture* iconTex, int iconW, int iconH);
    void updateDroppedItems(float px, float py, int pw, int ph);
    void renderDroppedItems(SDL_Renderer* renderer, int camX, int camY);
    void loadSavedDroppedItems();
    void saveDroppedItems() const;
    void saveMapTiles() const;
    void renderGroundPrompt(SDL_Renderer* renderer, int camX, int camY,
                            const Player& player);

    void updatePotionCollision(float px, float py, int pw, int ph);
    void renderPotionTiles(SDL_Renderer* renderer, int camX, int camY);
    void renderPotionPrompt(SDL_Renderer* renderer, int camX, int camY,
                            const Player& player);

    std::vector<std::string> tiles;
    int rows;
    int cols;

    Floor floor;
    Wall  wall;
    Door  door;
    TableEmpty tableEmpty;
    TablePaper tablePaper;
    Chair chair;
    Boockcase boockcase;
    Bag   bag;
    PotionHpMinor potionItem;

    Mix_Music* bgm;
    bool audioOpen;

    int activeDoorRow;
    int activeDoorCol;
    int activeTablePaperRow;
    int activeTablePaperCol;

    std::vector<DroppedItem> droppedItems;
    int activeGroundIdx;
    std::filesystem::path playerSavePath;

    SDL_Texture* paperIconTex;
    int          paperIconW;
    int          paperIconH;

    SDL_Texture* potionIconTex;
    int          potionIconW;
    int          potionIconH;

    SDL_Texture* noticeCollectTex;
    int          noticeCollectW;
    int          noticeCollectH;

    int activePotionRow;
    int activePotionCol;

    SDL_Texture* collectTextboxTex;
    int          collectTextboxW;
    int          collectTextboxH;

    SDL_Renderer* renderer;

    std::vector<std::unique_ptr<Ogro>> ogros;
    std::vector<std::pair<int,int>>    ogroSpawnPoints;
    int   ogroMaxCount;
    float ogroRespawnTime;
    float ogroRespawnTimer;
    std::vector<std::unique_ptr<Rat>> rats;
    std::vector<std::pair<int,int>> ratSpawnPoints;
    int ratMaxCount;
    float ratRespawnTime;
    float ratRespawnTimer;
};

#endif
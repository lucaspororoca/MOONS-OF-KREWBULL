#ifndef TELA_GAME_H
#define TELA_GAME_H

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <filesystem>
#include <vector>

#include "scripts/characters/player.h"
#include "scripts/maps/caves_of_chaos.h"
#include "scripts/hud/camera.h"
#include "scripts/characters/combat_enemy.h"

class TelaGame {
public:
    TelaGame(SDL_Renderer* renderer, int screenW, int screenH,
             const std::filesystem::path& savePath = {});
    ~TelaGame();

    bool init();
    void handleEvent(const SDL_Event& event);
    void update(float dt);
    void render();
    bool isRunning() const { return running; }
    void stop() { running = false; }
    bool hasPendingBattle() const { return battleRequested; }
    std::vector<CombatEnemy*> consumeBattleRequest();
    Player& getPlayer() { return player; }
    void resumeAfterBattle();
    void resumeAfterGameOver();

private:
    SDL_Renderer* renderer;
    int screenW;
    int screenH;
    bool running;
    std::filesystem::path requestedSavePath;
    float saveTimer;
    bool battleRequested;
    bool battleTriggered;
    std::vector<CombatEnemy*> battleEnemies;
    float battleStartX;
    float battleStartY;
    player_save::Character preBattleCharacter;
    bool hasPreBattleSnapshot;
    TTF_Font* progressionFont;

    CavesOfChaos map;
    Player player;
    Camera camera;
};

#endif
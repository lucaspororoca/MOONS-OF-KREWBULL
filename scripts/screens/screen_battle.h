#ifndef SCREEN_BATTLE_H
#define SCREEN_BATTLE_H

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <string>
#include <utility>
#include <vector>
#include "scripts/skills/paladino_devotion.h"
#include "scripts/skills/barbarian_berserker.h"

class Player;
class CombatEnemy;

class TelaBattle {
public:
    TelaBattle(SDL_Renderer* renderer, int screenW, int screenH,
               Player& player, const std::vector<CombatEnemy*>& enemies);
    ~TelaBattle();

    bool init();
    void handleEvent(const SDL_Event& event);
    void update(float dt);
    void render();

    bool isRunning() const { return running; }
    bool shouldReturnToGame() const { return returnToGame; }
    bool shouldShowGameOver() const { return gameOverRequested; }

private:
    bool loadTexture(const std::string& path, SDL_Texture*& texture);
    void destroyTextures();
    void renderText(const std::string& text, int x, int y, SDL_Color color);
    void renderCenteredText(const std::string& text, int centerX, int y,
                            SDL_Color color);
    void rollInitiative();
    void startBattle();
    void finishPlayerTurn(const std::string& usedSkill);
    void advanceTurn();
    void finishBattle();
    bool allEnemiesDefeated() const;
    void useLayingOnHands();
    void renderDevotionSkills();
    void renderBerserkerSkills();
    SDL_Rect skillHitbox(int buttonIndex) const;
    void addSkillBattleLog(const std::string& actorName,
                           const std::string& skillName,
                           const std::string& targetName);
    void renderBattleLog();
    void renderBattleControls();
    void renderAttributesOverlay();
    void renderInventoryOverlay();
    void toggleAttributesOverlay();
    void toggleInventoryOverlay();
    std::vector<std::string> wrappedBattleLogLines() const;

    SDL_Renderer* renderer;
    int screenW;
    int screenH;
    bool running;
    bool returnToGame;
    bool gameOverRequested;
    bool victorySequence;
    float victoryElapsed;
    int experienceEarned;
    int victoryFadeAlpha;
    int enemyCount;
    Player& player;
    std::vector<CombatEnemy*> enemies;
    paladino_devotion::BattleSkillController devotionSkills;
    barbarian_berserker::BattleSkillController berserkerSkills;
    int selectedTarget;
    bool playerTurn;
    bool waitingForBattleStart;
    bool attributesOpen;
    bool inventoryOpen;
    int turnNumber;
    std::size_t inventoryScrollOffset;
    struct TurnEntry {
        int participant;
        int initiative;
    };
    std::vector<TurnEntry> turnOrder;
    std::size_t activeTurn;
    std::vector<std::string> initiativeDetails;
    bool initiativeRolled;
    float opponentTurnTimer;
    std::string statusMessage;
    std::vector<std::string> battleLogs;
    std::size_t battleLogScrollOffset;
    TTF_Font* uiFont;

    SDL_Texture* scenarioTexture;
    SDL_Texture* playerTexture;
    SDL_Texture* playerSkillsTexture;
    SDL_Texture* ogreTexture;
    SDL_Texture* layingOnHandsTexture;
    SDL_Texture* sacredWeaponTexture;
    SDL_Texture* deathBlowTexture;
    SDL_Texture* crushingBlowTexture;
    SDL_Texture* shieldProtectionTexture;
    SDL_Texture* smashTexture;
    SDL_Texture* brutalCutTexture;
    SDL_Texture* fierceBeatTexture;
    SDL_Texture* yourTurnTexture;
    SDL_Texture* opponentTurnTexture;
    SDL_Texture* battleLogTexture;
    SDL_Texture* initiativeTexture;
    SDL_Texture* startBattleTexture;
    SDL_Texture* infoButtonTexture;
    SDL_Texture* bagButtonTexture;
    SDL_Texture* closeButtonTexture;
    SDL_Texture* attributesTexture;
    SDL_Texture* inventoryTexture;
    SDL_Texture* experienceNoticeTexture;
    SDL_Rect startBattleHitbox;
    SDL_Rect startBattleTextureRect;
};

#endif
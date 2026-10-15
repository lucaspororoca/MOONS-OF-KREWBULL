#include "scripts/screens/screen_battle.h"
#include "scripts/characters/player.h"
#include "scripts/characters/combat_enemy.h"
#include "scripts/dice/dice_1d20.h"
#include "scripts/hud/bgm_manager.h"
#include "scripts/resources/resource_archive.h"
#include "scripts/skills/paladino_devotion.h"
#include "scripts/skills/barbarian_berserker.h"
#include <algorithm>
#include <cmath>
#include <iomanip>
#include <iostream>
#include <sstream>

namespace {

std::string signedModifier(int value) {
    return (value >= 0 ? "+" : "-") + std::to_string(std::abs(value));
}

int abilityModifier(int score) {
    return static_cast<int>(std::floor((score - 10) / 2.0));
}

bool containsPoint(const SDL_Rect& rect, int x, int y) {
    return x >= rect.x && x < rect.x + rect.w &&
           y >= rect.y && y < rect.y + rect.h;
}

}

TelaBattle::TelaBattle(SDL_Renderer* renderer, int screenW, int screenH,
                       Player& player,
                       const std::vector<CombatEnemy*>& enemies)
    : renderer(renderer),
      screenW(screenW),
      screenH(screenH),
      running(true),
      returnToGame(false),
      gameOverRequested(false),
      victorySequence(false),
      victoryElapsed(0.0f),
      experienceEarned(0),
      victoryFadeAlpha(0),
      enemyCount(static_cast<int>(
          std::min<std::size_t>(enemies.size(), 1))),
      player(player),
      enemies(enemies.begin(), enemies.begin() + enemyCount),
      devotionSkills(),
      berserkerSkills(),
      selectedTarget(0),
      playerTurn(true),
      waitingForBattleStart(true),
      attributesOpen(false),
      inventoryOpen(false),
      turnNumber(1),
      inventoryScrollOffset(0),
      turnOrder(),
      activeTurn(0),
      initiativeDetails(),
      initiativeRolled(false),
      opponentTurnTimer(0.0f),
      statusMessage(),
      battleLogs(),
      battleLogScrollOffset(0),
      uiFont(nullptr),
      scenarioTexture(nullptr),
      playerTexture(nullptr),
      playerSkillsTexture(nullptr),
      ogreTexture(nullptr),
      layingOnHandsTexture(nullptr),
      sacredWeaponTexture(nullptr),
      deathBlowTexture(nullptr),
      crushingBlowTexture(nullptr),
      shieldProtectionTexture(nullptr),
      smashTexture(nullptr),
      brutalCutTexture(nullptr),
      fierceBeatTexture(nullptr),
      yourTurnTexture(nullptr),
      opponentTurnTexture(nullptr),
      battleLogTexture(nullptr),
      initiativeTexture(nullptr),
      startBattleTexture(nullptr),
      infoButtonTexture(nullptr),
      bagButtonTexture(nullptr),
      closeButtonTexture(nullptr),
      attributesTexture(nullptr),
      inventoryTexture(nullptr),
      experienceNoticeTexture(nullptr),
      startBattleHitbox{
          screenW * 622 / 1400, screenH * 552 / 850,
          screenW * 156 / 1400, screenH * 50 / 850},
      startBattleTextureRect{0, 0, 0, 0} {}

void TelaBattle::rollInitiative() {
    if (initiativeRolled) return;
    initiativeRolled = true;

    const std::string playerName =
        player.name.empty() ? "Jogador" : player.name;
    const int playerDexterityModifier =
        static_cast<int>(std::floor((player.dexterity - 10) / 2.0));
    const int playerDie = Dice1d20::roll();
    const int playerInitiative = playerDie + playerDexterityModifier;
    turnOrder.push_back({-1, playerInitiative});
    initiativeDetails.push_back(
        playerName + ": 1d20(" + std::to_string(playerDie) + ") " +
        (playerDexterityModifier >= 0 ? "+ " : "- ") +
        std::to_string(std::abs(playerDexterityModifier)) + " = " +
        std::to_string(playerInitiative));
    battleLogs.push_back(
        "Iniciativa " + initiativeDetails.back());

    for (int i = 0; i < enemyCount; ++i) {
        CombatEnemy* enemy = enemies[static_cast<std::size_t>(i)];
        if (!enemy) continue;
        const int die = Dice1d20::roll();
        const int modifier = abilityModifier(enemy->getDexterity());
        const int initiative = die + modifier;
        turnOrder.push_back({i, initiative});
        initiativeDetails.push_back(
            std::string(enemy->battleName()) + " " +
            std::to_string(i + 1) + ": 1d20(" +
            std::to_string(die) + ") " + (modifier >= 0 ? "+ " : "- ") +
            std::to_string(std::abs(modifier)) + " = " +
            std::to_string(initiative));
        battleLogs.push_back(
            "Iniciativa " + initiativeDetails.back());
    }
    std::stable_sort(turnOrder.begin(), turnOrder.end(),
        [](const TurnEntry& left, const TurnEntry& right) {
            return left.initiative > right.initiative;
        });
}

TelaBattle::~TelaBattle() {
    devotionSkills.endBattle(player);
    berserkerSkills.endBattle(player);
    destroyTextures();
    if (uiFont) TTF_CloseFont(uiFont);
}

bool TelaBattle::loadTexture(const std::string& path,
                            SDL_Texture*& texture) {
    SDL_Surface* surface = resource_archive::loadResourceImage(path);
    if (!surface) {
        std::cerr << "Falha ao carregar recurso de batalha " << path
                  << ": " << IMG_GetError() << std::endl;
        return false;
    }

    texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_FreeSurface(surface);
    if (!texture) {
        std::cerr << "Falha ao criar textura de batalha " << path
                  << ": " << SDL_GetError() << std::endl;
        return false;
    }
    SDL_SetTextureScaleMode(texture, SDL_ScaleModeNearest);
    return true;
}

void TelaBattle::destroyTextures() {
    if (scenarioTexture) SDL_DestroyTexture(scenarioTexture);
    if (playerTexture) SDL_DestroyTexture(playerTexture);
    if (playerSkillsTexture) SDL_DestroyTexture(playerSkillsTexture);
    if (ogreTexture) SDL_DestroyTexture(ogreTexture);
    if (layingOnHandsTexture) SDL_DestroyTexture(layingOnHandsTexture);
    if (sacredWeaponTexture) SDL_DestroyTexture(sacredWeaponTexture);
    if (deathBlowTexture) SDL_DestroyTexture(deathBlowTexture);
    if (crushingBlowTexture) SDL_DestroyTexture(crushingBlowTexture);
    if (shieldProtectionTexture) SDL_DestroyTexture(shieldProtectionTexture);
    if (smashTexture) SDL_DestroyTexture(smashTexture);
    if (brutalCutTexture) SDL_DestroyTexture(brutalCutTexture);
    if (fierceBeatTexture) SDL_DestroyTexture(fierceBeatTexture);
    if (yourTurnTexture) SDL_DestroyTexture(yourTurnTexture);
    if (opponentTurnTexture) SDL_DestroyTexture(opponentTurnTexture);
    if (battleLogTexture) SDL_DestroyTexture(battleLogTexture);
    if (initiativeTexture) SDL_DestroyTexture(initiativeTexture);
    if (startBattleTexture) SDL_DestroyTexture(startBattleTexture);
    if (infoButtonTexture) SDL_DestroyTexture(infoButtonTexture);
    if (bagButtonTexture) SDL_DestroyTexture(bagButtonTexture);
    if (closeButtonTexture) SDL_DestroyTexture(closeButtonTexture);
    if (attributesTexture) SDL_DestroyTexture(attributesTexture);
    if (inventoryTexture) SDL_DestroyTexture(inventoryTexture);
    if (experienceNoticeTexture) SDL_DestroyTexture(experienceNoticeTexture);
    scenarioTexture = nullptr;
    playerTexture = nullptr;
    playerSkillsTexture = nullptr;
    ogreTexture = nullptr;
    layingOnHandsTexture = nullptr;
    sacredWeaponTexture = nullptr;
    deathBlowTexture = nullptr;
    crushingBlowTexture = nullptr;
    shieldProtectionTexture = nullptr;
    smashTexture = nullptr;
    brutalCutTexture = nullptr;
    fierceBeatTexture = nullptr;
    yourTurnTexture = nullptr;
    opponentTurnTexture = nullptr;
    battleLogTexture = nullptr;
    initiativeTexture = nullptr;
    startBattleTexture = nullptr;
    infoButtonTexture = nullptr;
    bagButtonTexture = nullptr;
    closeButtonTexture = nullptr;
    attributesTexture = nullptr;
    inventoryTexture = nullptr;
    experienceNoticeTexture = nullptr;
}

void TelaBattle::startBattle() {
    if (!waitingForBattleStart) return;
    waitingForBattleStart = false;
    if (turnOrder.empty()) {
        finishBattle();
        return;
    }

    activeTurn = turnOrder.size() - 1;
    turnNumber = 0;
    advanceTurn();
}

bool TelaBattle::init() {
    uiFont = resource_archive::openResourceFont("resources/fonts/impact.ttf", 22);
    if (!uiFont) {
        std::cerr << "Falha ao carregar a fonte da interface de batalha: "
                  << TTF_GetError() << std::endl;
        return false;
    }

    if (!loadTexture(
            "resources/sprites/battle/scenario/scenario_caves_of_chaos.png",
            scenarioTexture)) {
        destroyTextures();
        return false;
    }
    if (enemyCount > 0 && enemies[0] &&
        !loadTexture(enemies[0]->battleSpritePath(), ogreTexture)) {
        destroyTextures();
        return false;
    }

    if ((player.characterClass == "paladin" ||
         player.characterClass == "paladino") &&
        player.gender == "female") {
        if (!loadTexture(
                "resources/sprites/battle/characters/player/paladino/female/"
                "battle_paladina_humana_mulher.png",
                playerTexture) ||
            !loadTexture(
                "resources/interfaces/scenes/battle/"
                "skills_paladino_humano_mulher.png",
                playerSkillsTexture)) {
            destroyTextures();
            return false;
        }
    }
    if ((player.characterClass == "barbarian" ||
         player.characterClass == "barbaro") &&
        player.gender == "female") {
        if (!loadTexture(
                "resources/sprites/battle/characters/player/barbaro/female/"
                "battle_barbaro_humana_mulher.png",
                playerTexture) ||
            !loadTexture(
                "resources/interfaces/scenes/battle/"
                "skills_barbaro_humano_mulher.png",
                playerSkillsTexture)) {
            destroyTextures();
            return false;
        }
    }

    const std::string layingHandsPath =
        devotionSkills.iconPath(player, "laying on of hands");
    if (!layingHandsPath.empty()) {
        if (!loadTexture(layingHandsPath, layingOnHandsTexture)) {
            destroyTextures();
            return false;
        }
    }
    const std::string sacredWeaponPath =
        devotionSkills.iconPath(player, "sacred weapon");
    if (!sacredWeaponPath.empty() &&
        !loadTexture(sacredWeaponPath, sacredWeaponTexture)) {
        destroyTextures();
        return false;
    }
    const std::string deathBlowPath =
        devotionSkills.iconPath(player, "death blow");
    if (!deathBlowPath.empty() &&
        !loadTexture(deathBlowPath, deathBlowTexture)) {
        destroyTextures();
        return false;
    }
    const std::string crushingBlowPath =
        devotionSkills.iconPath(player, "crushing blow");
    if (!crushingBlowPath.empty() &&
        !loadTexture(crushingBlowPath, crushingBlowTexture)) {
        destroyTextures();
        return false;
    }
    const std::string shieldProtectionPath =
        devotionSkills.iconPath(player, "shield protection");
    if (!shieldProtectionPath.empty() &&
        !loadTexture(shieldProtectionPath, shieldProtectionTexture)) {
        destroyTextures();
        return false;
    }

    const std::string smashPath =
        berserkerSkills.iconPath(player, "smash");
    if (!smashPath.empty() &&
        !loadTexture(smashPath, smashTexture)) {
        if (!loadTexture(
                "resources/icons/skills/barbaro/berserker/level1_smash.png",
                smashTexture)) {
            destroyTextures();
            return false;
        }
    }
    const std::string brutalCutPath =
        berserkerSkills.iconPath(player, "brutal cut");
    if (!brutalCutPath.empty() &&
        !loadTexture(brutalCutPath, brutalCutTexture)) {
        std::cerr << "Icone Brutal Cut indisponivel; exibindo botao textual."
                  << std::endl;
    }
    const std::string fierceBeatPath =
        berserkerSkills.iconPath(player, "fierce beat");
    if (!fierceBeatPath.empty() &&
        !loadTexture(fierceBeatPath, fierceBeatTexture)) {
        std::cerr << "Icone Fierce Beat indisponivel; exibindo botao textual."
                  << std::endl;
    }

    if (!loadTexture(
            "resources/interfaces/scenes/battle/hud_your_turn.png",
            yourTurnTexture) ||
        !loadTexture(
            "resources/interfaces/scenes/battle/hud_opponent_turn.png",
            opponentTurnTexture) ||
        !loadTexture(
            "resources/interfaces/scenes/battle/textbox_battle_log.png",
            battleLogTexture)) {
                destroyTextures();
                return false;
    }
    if (!loadTexture(
            "resources/interfaces/notices/notice_earn_xp.png",
            experienceNoticeTexture)) {
        destroyTextures();
        return false;
    }

    if (!loadTexture(
            "resources/interfaces/textbox/textbox_initiative.png",
            initiativeTexture)) {
        std::cerr << "Usando painel de iniciativa desenhado: "
                  << "textbox_initiative.png nao encontrado." << std::endl;
    }
    if (!loadTexture(
            "resources/interfaces/buttons/button_start_battle.png",
            startBattleTexture)) {
        std::cerr << "Usando botao de inicio desenhado: "
                  << "button_start_battle.png nao encontrado." << std::endl;
    } else {
        int textureWidth = 0;
        int textureHeight = 0;
        if (SDL_QueryTexture(startBattleTexture, nullptr, nullptr,
                             &textureWidth, &textureHeight) != 0) {
            std::cerr << "Falha ao consultar o tamanho do botao de inicio: "
                      << SDL_GetError() << std::endl;
            destroyTextures();
            return false;
        }
        startBattleTextureRect = {
            (screenW - textureWidth) / 2,
            (screenH - textureHeight) / 2,
            textureWidth, textureHeight};
        const double scaleX = textureWidth / 1400.0;
        const double scaleY = textureHeight / 850.0;
        startBattleHitbox = {
            startBattleTextureRect.x +
                static_cast<int>(622 * scaleX),
            startBattleTextureRect.y +
                static_cast<int>(552 * scaleY),
            static_cast<int>(156 * scaleX),
            static_cast<int>(50 * scaleY)};
    }

    if (!loadTexture(
            "resources/interfaces/buttons/button_info.png",
            infoButtonTexture) ||
        !loadTexture(
            "resources/interfaces/buttons/button_bag_2.png",
            bagButtonTexture) ||
        !loadTexture(
            "resources/interfaces/buttons/button_close.png",
            closeButtonTexture) ||
        !loadTexture(
            "resources/interfaces/textbox/textbox_screen_attributes.png",
            attributesTexture) ||
        !loadTexture(
            "resources/interfaces/textbox/textbox_inventory.png",
            inventoryTexture)) {
        destroyTextures();
        return false;
    }

    if (!bgm_manager::playLoop("resources/audio/bgm/battle/battle_01.mp3")) {
        destroyTextures();
        return false;
    }

    rollInitiative();
    return true;
}

void TelaBattle::handleEvent(const SDL_Event& event) {
    if (event.type == SDL_QUIT) {
        running = false;
        return;
    }
    if (victorySequence) return;
    if (waitingForBattleStart) {
        if (event.type == SDL_MOUSEBUTTONDOWN &&
            event.button.button == SDL_BUTTON_LEFT &&
            event.button.x >= startBattleHitbox.x &&
            event.button.x < startBattleHitbox.x + startBattleHitbox.w &&
            event.button.y >= startBattleHitbox.y &&
            event.button.y < startBattleHitbox.y + startBattleHitbox.h) {
            startBattle();
        }
        return;
    }
    if (attributesOpen || inventoryOpen) {
        if (event.type == SDL_MOUSEBUTTONDOWN &&
            event.button.button == SDL_BUTTON_LEFT) {
            const SDL_Rect closeHitbox = {
                screenW * 900 / 1400, screenH * 182 / 850,
                screenW * 28 / 1400, screenH * 28 / 850};
            if (containsPoint(closeHitbox, event.button.x, event.button.y)) {
                attributesOpen = false;
                inventoryOpen = false;
            }
        }
        if (event.type == SDL_MOUSEWHEEL && inventoryOpen) {
            const int direction =
                event.wheel.direction == SDL_MOUSEWHEEL_FLIPPED
                    ? -event.wheel.y : event.wheel.y;
            const std::size_t visibleRows = 18;
            const std::size_t maxOffset =
                player.inventory.size() > visibleRows
                    ? player.inventory.size() - visibleRows : 0;
            if (direction > 0) {
                inventoryScrollOffset = std::min(
                    maxOffset, inventoryScrollOffset +
                        static_cast<std::size_t>(direction));
            } else if (direction < 0) {
                const std::size_t amount =
                    static_cast<std::size_t>(-direction);
                inventoryScrollOffset =
                    inventoryScrollOffset > amount
                        ? inventoryScrollOffset - amount : 0;
            }
        }
        if (event.type == SDL_KEYDOWN &&
            event.key.keysym.scancode == SDL_SCANCODE_ESCAPE) {
            attributesOpen = false;
            inventoryOpen = false;
        }
        return;
    }
    if (event.type == SDL_MOUSEBUTTONDOWN &&
        event.button.button == SDL_BUTTON_LEFT) {
        const SDL_Rect infoHitbox = {
            screenW * 439 / 1400, screenH * 729 / 850,
            screenW * 82 / 1400, screenH * 36 / 850};
        const SDL_Rect bagHitbox = {
            screenW * 439 / 1400, screenH * 769 / 850,
            screenW * 82 / 1400, screenH * 36 / 850};
        if (containsPoint(infoHitbox, event.button.x, event.button.y)) {
            toggleAttributesOverlay();
            return;
        }
        if (containsPoint(bagHitbox, event.button.x, event.button.y)) {
            toggleInventoryOverlay();
            return;
        }
    }
    if (event.type == SDL_MOUSEWHEEL) {
        const std::vector<std::string> lines = wrappedBattleLogLines();
        const std::size_t maxLines = 7;
        const std::size_t maxOffset =
            lines.size() > maxLines ? lines.size() - maxLines : 0;
        const int wheelY = event.wheel.direction == SDL_MOUSEWHEEL_FLIPPED
            ? -event.wheel.y : event.wheel.y;
        if (wheelY > 0) {
            battleLogScrollOffset = std::min(
                maxOffset, battleLogScrollOffset +
                    static_cast<std::size_t>(wheelY * 2));
        } else if (wheelY < 0) {
            const std::size_t scrollDown =
                static_cast<std::size_t>(-wheelY * 2);
            battleLogScrollOffset = battleLogScrollOffset > scrollDown
                ? battleLogScrollOffset - scrollDown : 0;
        }
        return;
    }
    if (event.type == SDL_MOUSEBUTTONDOWN &&
        event.button.button == SDL_BUTTON_LEFT &&
        (smashTexture || berserkerSkills.isAvailable(player, "smash"))) {
        const SDL_Rect hitbox = skillHitbox(0);
        if (event.button.x >= hitbox.x &&
            event.button.x < hitbox.x + hitbox.w &&
            event.button.y >= hitbox.y &&
            event.button.y < hitbox.y + hitbox.h) {
            if (!playerTurn) {
                statusMessage = "A skill so pode ser usada no turno do jogador.";
            } else if (berserkerSkills.cooldownRemaining("smash") > 0) {
                statusMessage = "smash disponivel em " +
                    std::to_string(
                        berserkerSkills.cooldownRemaining("smash")) +
                    " turno(s).";
            } else {
                int target = selectedTarget - 1;
                if (target < 0 || target >= enemyCount ||
                    !enemies[static_cast<std::size_t>(target)]->isAlive()) {
                    target = 0;
                    while (target < enemyCount &&
                           !enemies[static_cast<std::size_t>(target)]->isAlive()) {
                        ++target;
                    }
                }
                if (target >= enemyCount) {
                    statusMessage = "Nao ha inimigos vivos.";
                    return;
                }
                selectedTarget = target + 1;
                int attackRoll = 0;
                int damage = 0;
                int damageDie = 0;
                int strengthModifier = 0;
                CombatEnemy& enemy =
                    *enemies[static_cast<std::size_t>(target)];
                const int previousHp = enemy.getHP();
                if (berserkerSkills.useSmash(
                        player, enemy, attackRoll, damage,
                        damageDie, strengthModifier)) {
                    addSkillBattleLog(
                        player.name.empty() ? "Jogador" : player.name,
                        "Smash",
                        std::string(enemy.battleName()) + " " +
                            std::to_string(target + 1));
                    battleLogs.push_back(
                        damage > 0
                            ? "Dano 1d8(" + std::to_string(damageDie) +
                                ") + mod FOR (" +
                                signedModifier(strengthModifier) + ") = " +
                                std::to_string(damage) +
                                "; HP removido: " +
                                std::to_string(previousHp - enemy.getHP())
                            : "Ataque 1d20 total " +
                                std::to_string(attackRoll) + " vs CA " +
                                std::to_string(enemy.getArmorClass()) +
                                "; HP removido: 0");
                    statusMessage = damage > 0
                        ? "Smash acertou (" +
                            std::to_string(attackRoll) + "), dano " +
                            std::to_string(damage) + "."
                        : "Smash errou (" +
                            std::to_string(attackRoll) + " contra CA " +
                            std::to_string(enemy.getArmorClass()) + ").";
                    if (allEnemiesDefeated()) {
                        finishBattle();
                    } else {
                        finishPlayerTurn("smash");
                    }
                }
            }
            return;
        }
    }
    if (event.type == SDL_MOUSEBUTTONDOWN &&
        event.button.button == SDL_BUTTON_LEFT &&
        (berserkerSkills.isAvailable(player, "brutal cut") ||
         berserkerSkills.isAvailable(player, "fierce beat"))) {
        const int slot = containsPoint(skillHitbox(1),
                                       event.button.x, event.button.y) ? 1
            : (containsPoint(skillHitbox(2),
                             event.button.x, event.button.y) ? 2 : -1);
        if (slot >= 0) {
            const bool brutalCut = slot == 1;
            const std::string skillName =
                brutalCut ? "brutal cut" : "fierce beat";
            const std::string skillLabel =
                brutalCut ? "Brutal Cut" : "Fierce Beat";
            if (!playerTurn) {
                statusMessage = "A skill so pode ser usada no turno do jogador.";
            } else if (berserkerSkills.cooldownRemaining(skillName) > 0) {
                statusMessage = skillLabel + " disponivel em " +
                    std::to_string(
                        berserkerSkills.cooldownRemaining(skillName)) +
                    " turno(s).";
            } else {
                int target = selectedTarget - 1;
                if (target < 0 || target >= enemyCount ||
                    !enemies[static_cast<std::size_t>(target)]->isAlive()) {
                    target = 0;
                    while (target < enemyCount &&
                           !enemies[static_cast<std::size_t>(target)]->isAlive()) {
                        ++target;
                    }
                }
                if (target >= enemyCount) {
                    statusMessage = "Nao ha inimigos vivos.";
                    return;
                }
                selectedTarget = target + 1;
                CombatEnemy& enemy =
                    *enemies[static_cast<std::size_t>(target)];
                const int previousHp = enemy.getHP();
                int attackRoll = 0;
                int damage = 0;
                int firstDie = 0;
                int secondDie = 0;
                int discardedDie = 0;
                int strengthModifier = 0;
                const bool used = brutalCut
                    ? berserkerSkills.useBrutalCut(
                          player, enemy, attackRoll, damage, firstDie,
                          secondDie, strengthModifier)
                    : berserkerSkills.useFierceBeat(
                          player, enemy, attackRoll, damage, firstDie,
                          secondDie, discardedDie, strengthModifier);
                if (used) {
                    const std::string targetName =
                        std::string(enemy.battleName()) + " " +
                        std::to_string(target + 1);
                    addSkillBattleLog(
                        player.name.empty() ? "Jogador" : player.name,
                        skillLabel, targetName);
                    if (damage > 0) {
                        const std::string diceLog = brutalCut
                            ? "2d8(" + std::to_string(firstDie) + "+" +
                                  std::to_string(secondDie) + ")"
                            : "2d10(" + std::to_string(firstDie) + "+" +
                                  std::to_string(secondDie) + ", descartado " +
                                  std::to_string(discardedDie) + ")";
                        battleLogs.push_back(
                            diceLog + " + mod FOR (" +
                            signedModifier(strengthModifier) + ") = " +
                            std::to_string(damage) + "; HP removido: " +
                            std::to_string(previousHp - enemy.getHP()));
                        statusMessage = skillLabel + " acertou (" +
                            std::to_string(attackRoll) + "), dano " +
                            std::to_string(damage) + ".";
                    } else {
                        battleLogs.push_back(
                            "Ataque 1d20 total " +
                            std::to_string(attackRoll) + " vs CA " +
                            std::to_string(enemy.getArmorClass()) +
                            "; HP removido: 0");
                        statusMessage = skillLabel + " errou (" +
                            std::to_string(attackRoll) + " contra CA " +
                            std::to_string(enemy.getArmorClass()) + ").";
                    }
                    if (allEnemiesDefeated()) {
                        finishBattle();
                    } else {
                        finishPlayerTurn(skillName);
                    }
                }
            }
            return;
        }
    }
    if (event.type == SDL_MOUSEBUTTONDOWN &&
        event.button.button == SDL_BUTTON_LEFT &&
        (layingOnHandsTexture || sacredWeaponTexture ||
         deathBlowTexture || crushingBlowTexture || shieldProtectionTexture ||
         devotionSkills.isAvailable(player, "shield protection"))) {
        for (int button = 0; button < 5; ++button) {
            SDL_Texture* skillTexture = button == 0
                ? layingOnHandsTexture
                : (button == 1 ? sacredWeaponTexture
                   : (button == 2 ? deathBlowTexture
                      : (button == 3 ? crushingBlowTexture
                                     : shieldProtectionTexture)));
            if (!skillTexture) continue;
            const std::string skillName =
                button == 0 ? "laying on of hands"
                : (button == 1 ? "sacred weapon"
                   : (button == 2 ? "death blow"
                      : (button == 3 ? "crushing blow"
                                     : "shield protection")));
            const SDL_Rect hitbox = skillHitbox(button);
            if (event.button.x < hitbox.x ||
                event.button.x >= hitbox.x + hitbox.w ||
                event.button.y < hitbox.y ||
                event.button.y >= hitbox.y + hitbox.h) {
                continue;
            }
            if (!playerTurn) {
                statusMessage = "A skill so pode ser usada no turno do jogador.";
            } else if (devotionSkills.cooldownRemaining(skillName) > 0) {
                statusMessage = skillName + " disponivel em " +
                    std::to_string(
                        devotionSkills.cooldownRemaining(skillName)) +
                    " turno(s).";
            } else if (button == 0) {
                useLayingOnHands();
            } else if (button == 1) {
                if (devotionSkills.useSacredWeapon(player)) {
                    addSkillBattleLog(
                        player.name.empty() ? "Jogador" : player.name,
                        "Sacred Weapon",
                        player.name.empty() ? "Jogador" : player.name);
                    finishPlayerTurn("sacred weapon");
                }
            } else if (button == 2 || button == 3) {
                int target = selectedTarget - 1;
                if (target < 0 || target >= enemyCount ||
                    !enemies[static_cast<std::size_t>(target)]->isAlive()) {
                    target = 0;
                    while (target < enemyCount &&
                           !enemies[static_cast<std::size_t>(target)]->isAlive()) {
                        ++target;
                    }
                }
                if (target >= enemyCount) {
                    statusMessage = "Nao ha inimigos vivos.";
                    return;
                }
                selectedTarget = target + 1;
                int attackRoll = 0;
                int damage = 0;
                int damageDie = 0;
                int strengthModifier = 0;
                CombatEnemy& enemy =
                    *enemies[static_cast<std::size_t>(target)];
                const int previousHp = enemy.getHP();
                const bool deathBlow = button == 2;
                const bool usedSkill = deathBlow
                    ? devotionSkills.useDeathBlow(
                          player, enemy, attackRoll, damage,
                          damageDie, strengthModifier)
                    : devotionSkills.useCrushingBlow(
                          player, enemy, attackRoll, damage,
                          damageDie, strengthModifier);
                if (usedSkill) {
                    const std::string skillLabel =
                        deathBlow ? "Death Blow" : "Crushing Blow";
                    const std::string skillName =
                        deathBlow ? "death blow" : "crushing blow";
                    addSkillBattleLog(
                        player.name.empty() ? "Jogador" : player.name,
                        skillLabel,
                        std::string(enemy.battleName()) + " " +
                            std::to_string(target + 1));
                    if (damage > 0) {
                        battleLogs.push_back(
                            (deathBlow ? "Death Blow: " : "Crushing Blow: ") +
                            std::string("Dano 1d8(") +
                            std::to_string(damageDie) + ") + mod FOR (" +
                            signedModifier(strengthModifier) + ") = " +
                            std::to_string(damage) + "; HP removido: " +
                            std::to_string(previousHp - enemy.getHP()));
                    } else {
                        battleLogs.push_back(
                            "Ataque 1d20 total " +
                            std::to_string(attackRoll) + " vs CA " +
                            std::to_string(enemy.getArmorClass()) +
                            "; HP removido: 0");
                    }
                    statusMessage = damage > 0
                        ? (deathBlow
                               ? skillLabel + " acertou, dano " +
                                     std::to_string(damage) + "."
                               : skillLabel + " acertou (" +
                                     std::to_string(attackRoll) + "), dano " +
                                     std::to_string(damage) + ".")
                        : skillLabel + " errou (" +
                            std::to_string(attackRoll) + " contra CA " +
                            std::to_string(enemy.getArmorClass()) + ").";
                    if (allEnemiesDefeated()) {
                        finishBattle();
                    } else {
                        finishPlayerTurn(skillName);
                    }
                } else {
                    statusMessage = (deathBlow ? "Death Blow" : "Crushing Blow") +
                        std::string(" requer uma arma equipada disponivel.");
                }
            } else if (devotionSkills.useShieldProtection(player)) {
                const std::string playerName =
                    player.name.empty() ? "Jogador" : player.name;
                addSkillBattleLog(playerName, "Shield Protection", playerName);
                statusMessage =
                    "Shield Protection protegara voce do proximo ataque.";
                finishPlayerTurn("shield protection");
            }
            return;
        }
    }
    if (event.type != SDL_KEYDOWN || event.key.repeat != 0) return;

    const SDL_Scancode key = event.key.keysym.scancode;
    if (key == SDL_SCANCODE_ESCAPE) {
        if (attributesOpen || inventoryOpen) {
            attributesOpen = false;
            inventoryOpen = false;
            return;
        }
        returnToGame = true;
        return;
    }

    if (key == SDL_SCANCODE_L) {
        if (!devotionSkills.isAvailable(player, "laying on of hands")) {
            statusMessage = "Este personagem nao possui Laying on Hands.";
            return;
        }
        if (!playerTurn) {
            statusMessage = "A skill so pode ser usada no turno do jogador.";
            return;
        }
        if (devotionSkills.cooldownRemaining("laying on of hands") > 0) {
            statusMessage = "Laying on Hands disponivel em " +
                std::to_string(devotionSkills.cooldownRemaining(
                    "laying on of hands")) + " turno(s).";
            return;
        }
        useLayingOnHands();
        return;
    }
    if (key == SDL_SCANCODE_S) {
        if (!devotionSkills.isAvailable(player, "sacred weapon")) {
            statusMessage = "Este personagem nao possui Sacred Weapon.";
            return;
        }
        if (!playerTurn) {
            statusMessage = "A skill so pode ser usada no turno do jogador.";
            return;
        }
        if (devotionSkills.cooldownRemaining("sacred weapon") > 0) {
            statusMessage = "Sacred Weapon disponivel em " +
                std::to_string(devotionSkills.cooldownRemaining(
                    "sacred weapon")) + " turno(s).";
            return;
        }
        if (devotionSkills.useSacredWeapon(player)) {
            addSkillBattleLog(
                player.name.empty() ? "Jogador" : player.name,
                "Sacred Weapon",
                player.name.empty() ? "Jogador" : player.name);
            finishPlayerTurn("sacred weapon");
        }
        return;
    }
    if (!playerTurn) return;
}

void TelaBattle::update(float dt) {
    if (victorySequence) {
        victoryElapsed += dt;
        if (victoryElapsed > 6.0f) {
            victoryFadeAlpha = std::min(
                255, static_cast<int>((victoryElapsed - 6.0f) * 255.0f));
        }
        if (victoryElapsed >= 7.0f) returnToGame = true;
        return;
    }
    if (waitingForBattleStart) return;
    if (!playerTurn) {
        opponentTurnTimer -= dt;
        if (opponentTurnTimer <= 0.0f) {
            CombatEnemy* enemy = enemies[static_cast<std::size_t>(
                turnOrder[activeTurn].participant)];
            if (!enemy || !enemy->isAlive()) {
                advanceTurn();
                return;
            }
            const int originalArmorClass = player.armorClass;
            const bool shieldProtectionActive =
                devotionSkills.shieldProtectionPending();
            const int attackArmorClass =
                originalArmorClass + (shieldProtectionActive ? 3 : 0);
            if (shieldProtectionActive) {
                player.armorClass += 3;
            }
            const CombatEnemy::BattleTurnResult result =
                enemy->takeBattleTurn(player);
            if (shieldProtectionActive) {
                player.armorClass = originalArmorClass;
                devotionSkills.resolveShieldProtection();
            }
            const std::string enemyName =
                std::string(enemy->battleName()) + " " +
                std::to_string(turnOrder[activeTurn].participant + 1);
            if (result.moveName.empty()) {
                enemy->endBattleTurn();
                advanceTurn();
                return;
            }
            addSkillBattleLog(enemyName, result.moveName,
                result.healing ? enemyName
                               : (player.name.empty() ? "Jogador" : player.name));
            if (result.healing) {
                statusMessage = enemyName + " usou Fatigue e recuperou " +
                    std::to_string(result.amount) + " HP.";
                battleLogs.push_back(
                    "1d4(" + std::to_string(result.healingDie) +
                    ") + mod CON (" +
                    signedModifier(result.constitutionModifier) + ") = +" +
                    std::to_string(result.amount) +
                    "; HP atual do ogro: " +
                    std::to_string(enemy->getHP()));
            } else if (result.hit) {
                statusMessage = enemyName + " acertou " + result.moveName +
                    ", dano " +
                    std::to_string(result.amount) + ".";
                if (result.moveName == "Bite") {
                    battleLogs.push_back(
                        "Ataque 1d20 total " + std::to_string(result.roll) +
                        " vs CA " + std::to_string(attackArmorClass) +
                        "; dano 1 + 1d4(" +
                        std::to_string(result.damageDie) + ") = " +
                        std::to_string(result.amount) +
                        "; HP removido: " + std::to_string(result.hpLost));
                } else {
                    battleLogs.push_back(
                        "Dano 2d8(" +
                        std::to_string(result.discardedDamageDie) +
                        " descartado, " + std::to_string(result.damageDie) +
                        " mantido) + 4 = " +
                        std::to_string(result.amount) +
                        "; HP removido: " + std::to_string(result.hpLost));
                }
            } else {
                statusMessage = enemyName + " errou " + result.moveName +
                    " (" +
                    std::to_string(result.roll) + " contra CA " +
                    std::to_string(attackArmorClass) + ").";
                battleLogs.push_back(
                    "Ataque 1d20 total " + std::to_string(result.roll) +
                    " vs CA " + std::to_string(attackArmorClass) +
                    "; HP removido: 0");
            }
            enemy->endBattleTurn();
            if (player.hp <= 0) {
                finishBattle();
                return;
            }
            advanceTurn();
        }
    }
}

void TelaBattle::useLayingOnHands() {
    int healedAmount = 0;
    if (!devotionSkills.useLayingOnHands(player, healedAmount)) {
        statusMessage = player.hp >= player.maxHp
            ? "Seu HP ja esta cheio."
            : "Laying on Hands esta em cooldown.";
        return;
    }
    const std::string playerName =
        player.name.empty() ? "Jogador" : player.name;
    addSkillBattleLog(playerName, "Laying on Hands", playerName);
    battleLogs.push_back(
        "Recuperou +" + std::to_string(healedAmount) + " HP (" +
        std::to_string(player.hp) + "/" + std::to_string(player.maxHp) + ").");
    statusMessage = "Laying on Hands recuperou +" +
        std::to_string(healedAmount) + " HP.";
    finishPlayerTurn("laying on of hands");
}

SDL_Rect TelaBattle::skillHitbox(int buttonIndex) const {
    const int sourceX[] = {140, 189, 237, 286, 335};
    const int x = sourceX[buttonIndex] * screenW / 1400;
    const int y = 737 * screenH / 850;
    const int right = (sourceX[buttonIndex] + 39) * screenW / 1400;
    const int bottom = 776 * screenH / 850;
    return {x, y, right - x, bottom - y};
}

void TelaBattle::finishPlayerTurn(const std::string& usedSkill) {
    devotionSkills.finishPlayerTurn(player, usedSkill);
    berserkerSkills.finishPlayerTurn(player, usedSkill);
    advanceTurn();
}

void TelaBattle::advanceTurn() {
    if (allEnemiesDefeated() || player.hp <= 0 || turnOrder.empty()) {
        finishBattle();
        return;
    }

    for (std::size_t checked = 0; checked < turnOrder.size(); ++checked) {
        activeTurn = (activeTurn + 1) % turnOrder.size();
        if (activeTurn == 0) ++turnNumber;
        const int participant = turnOrder[activeTurn].participant;
        if (participant == -1) {
            playerTurn = true;
            statusMessage = "Seu turno.";
            devotionSkills.beginPlayerTurn(player);
            berserkerSkills.beginPlayerTurn(player);
            return;
        }
        if (participant < 0 ||
            static_cast<std::size_t>(participant) >= enemies.size()) {
            continue;
        }
        CombatEnemy* enemy = enemies[static_cast<std::size_t>(participant)];
        if (!enemy || !enemy->isAlive()) continue;
        enemy->beginBattleTurn();
        if (!enemy->hasAvailableBattleMove()) {
            enemy->endBattleTurn();
            continue;
        }
        playerTurn = false;
        opponentTurnTimer = 0.8f;
        statusMessage = "Turno do " + std::string(enemy->battleName()) + " " +
            std::to_string(participant + 1) + ".";
        return;
    }
    finishBattle();
}

void TelaBattle::finishBattle() {
    if (player.hp <= 0) {
        gameOverRequested = true;
    } else if (allEnemiesDefeated()) {
        if (!victorySequence) {
            for (const CombatEnemy* enemy : enemies) {
                if (enemy && !enemy->isAlive()) {
                    experienceEarned += enemy->getExperienceReward();
                }
            }
            const int levelsGained = player.gainExperience(experienceEarned);
            battleLogs.push_back(
                "Voce ganhou +" + std::to_string(experienceEarned) + " XP.");
            if (levelsGained > 0) {
                battleLogs.push_back(
                    "Level aumentado para " + std::to_string(player.level) + ".");
            }
            victorySequence = true;
            victoryElapsed = 0.0f;
        }
    } else {
        returnToGame = true;
    }
    playerTurn = false;
    devotionSkills.endBattle(player);
    berserkerSkills.endBattle(player);
}

bool TelaBattle::allEnemiesDefeated() const {
    return std::all_of(enemies.begin(), enemies.end(),
        [](const CombatEnemy* enemy) {
            return !enemy || !enemy->isAlive();
        });
}

void TelaBattle::addSkillBattleLog(const std::string& actorName,
                                   const std::string& skillName,
                                   const std::string& targetName) {
    battleLogs.push_back(actorName + " usou " + skillName +
                         " no " + targetName);
}

void TelaBattle::renderText(const std::string& text, int x, int y,
                            SDL_Color color) {
    if (!uiFont) return;
    SDL_Surface* surface = TTF_RenderUTF8_Blended(uiFont, text.c_str(), color);
    if (!surface) {
        std::cerr << "Falha ao renderizar texto da batalha: "
                  << TTF_GetError() << std::endl;
        return;
    }
    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
    if (!texture) {
        std::cerr << "Falha ao criar texto da batalha: "
                  << SDL_GetError() << std::endl;
        SDL_FreeSurface(surface);
        return;
    }
    const SDL_Rect destination = {x, y, surface->w, surface->h};
    SDL_FreeSurface(surface);
    SDL_RenderCopy(renderer, texture, nullptr, &destination);
    SDL_DestroyTexture(texture);
}

void TelaBattle::renderCenteredText(const std::string& text, int centerX,
                                    int y, SDL_Color color) {
    if (!uiFont) return;
    int textWidth = 0;
    if (TTF_SizeUTF8(uiFont, text.c_str(), &textWidth, nullptr) != 0) {
        std::cerr << "Falha ao medir texto da batalha: "
                  << TTF_GetError() << std::endl;
        return;
    }
    renderText(text, centerX - textWidth / 2, y, color);
}

void TelaBattle::render() {
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);

    const SDL_Rect screenRect = {0, 0, screenW, screenH};
    SDL_RenderCopy(renderer, scenarioTexture, nullptr, &screenRect);
    if (ogreTexture) {
        if (enemyCount > 0 && enemies[0] &&
            enemies[0]->battleSpriteFillsScreen()) {
            SDL_RenderCopy(renderer, ogreTexture, nullptr, &screenRect);
        } else {
            int textureWidth = 0;
            int textureHeight = 0;
            if (SDL_QueryTexture(ogreTexture, nullptr, nullptr,
                                 &textureWidth, &textureHeight) == 0 &&
                                 textureWidth > 0 && textureHeight > 0) {
                const int destinationHeight = screenH * 45 / 100;
                const int destinationWidth = textureWidth *
                    destinationHeight / textureHeight;
                const SDL_Rect enemyRect = {
                    screenW * 72 / 100 - destinationWidth / 2,
                    screenH * 58 / 100 - destinationHeight / 2,
                    destinationWidth, destinationHeight};
                SDL_RenderCopy(renderer, ogreTexture, nullptr, &enemyRect);
            } else {
                std::cerr << "Falha ao consultar o sprite do inimigo: "
                          << SDL_GetError() << std::endl;
            }
        }
    }
    if (playerTexture) {
        SDL_RenderCopy(renderer, playerTexture, nullptr, &screenRect);
    }
    if (playerSkillsTexture) {
        SDL_RenderCopy(renderer, playerSkillsTexture, nullptr, &screenRect);
    }

    SDL_RenderCopy(renderer, playerTurn ? yourTurnTexture : opponentTurnTexture,
                    nullptr, &screenRect);
    SDL_RenderCopy(renderer, battleLogTexture, nullptr, &screenRect);

    if (waitingForBattleStart) {
        if (initiativeTexture) {
            SDL_RenderCopy(renderer, initiativeTexture, nullptr, &screenRect);
        } else {
            const SDL_Rect initiativePanel = {
                screenW / 5, screenH / 4,
                screenW * 3 / 5, screenH / 2};
            SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
            SDL_SetRenderDrawColor(renderer, 15, 12, 8, 235);
            SDL_RenderFillRect(renderer, &initiativePanel);
            SDL_SetRenderDrawColor(renderer, 255, 198, 1, 255);
            SDL_RenderDrawRect(renderer, &initiativePanel);
            SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
        }

        const SDL_Color textColor = {255, 198, 1, 255};
        const int initiativeLineHeight = std::max(24, screenH * 4 / 100);
        const int firstLineY = screenH * 39 / 100;
        for (std::size_t i = 0; i < initiativeDetails.size(); ++i) {
            renderCenteredText(
                initiativeDetails[i], screenW / 2,
                firstLineY + static_cast<int>(i) * initiativeLineHeight,
                textColor);
        }

        if (startBattleTexture) {
            SDL_RenderCopy(renderer, startBattleTexture, nullptr,
                           &startBattleTextureRect);
        } else {
            SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
            SDL_SetRenderDrawColor(renderer, 75, 52, 18, 255);
            SDL_RenderFillRect(renderer, &startBattleHitbox);
            SDL_SetRenderDrawColor(renderer, 255, 198, 1, 255);
            SDL_RenderDrawRect(renderer, &startBattleHitbox);
            SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
            renderCenteredText("Iniciar batalha",
                startBattleHitbox.x + startBattleHitbox.w / 2,
                startBattleHitbox.y +
                    (startBattleHitbox.h - TTF_FontHeight(uiFont)) / 2,
                textColor);
        }
        return;
    }

    renderDevotionSkills();
    renderBerserkerSkills();
    renderBattleLog();
    const SDL_Color textColor = {255, 198, 1, 255};
    renderText("HP " + (player.name.empty() ? std::string("Jogador")
                                             : player.name) + ": " +
                   std::to_string(player.hp) + "/" +
                   std::to_string(player.maxHp),
               screenW * 18 / 1400, screenH * 18 / 850, textColor);
    const int displayedArmorClass = player.armorClass +
        (devotionSkills.shieldProtectionPending() ? 3 : 0);
    renderText("CA: " + std::to_string(displayedArmorClass),
               screenW * 18 / 1400, screenH * 44 / 850, textColor);
    for (int i = 0; i < enemyCount; ++i) {
        const CombatEnemy* enemy = enemies[static_cast<std::size_t>(i)];
        if (!enemy) continue;
        renderText(std::string(enemy->battleName()) + " " +
                       std::to_string(i + 1) + " CA: " +
                       std::to_string(enemy->getArmorClass()) + " HP: " +
                       std::to_string(enemy->getHP()) + "/" +
                       std::to_string(enemy->getMaxHP()),
                   screenW * 1100 / 1400,
                   screenH * (18 + i * 28) / 850, textColor);
    }
    renderText(statusMessage, screenW * 18 / 1400,
               screenH * 690 / 850, textColor);
    renderCenteredText("" + std::to_string(turnNumber),
                       screenW / 2, screenH * 45 / 850, textColor);
    renderBattleControls();
    if (attributesOpen) renderAttributesOverlay();
    if (inventoryOpen) renderInventoryOverlay();

    if (victorySequence) {
        if (experienceNoticeTexture) {
            SDL_RenderCopy(renderer, experienceNoticeTexture, nullptr, &screenRect);
        }
        renderCenteredText(
            "You earn +" + std::to_string(experienceEarned) + " XP",
            screenW / 2, screenH / 2, {255, 198, 1, 255});
        if (victoryFadeAlpha > 0) {
            SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
            SDL_SetRenderDrawColor(renderer, 0, 0, 0,
                                   static_cast<Uint8>(victoryFadeAlpha));
            SDL_RenderFillRect(renderer, &screenRect);
            SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
        }
    }
}

void TelaBattle::renderDevotionSkills() {
    const SDL_Rect screenRect = {0, 0, screenW, screenH};
    const SDL_Color textColor = {255, 198, 1, 255};
    const std::string skillNames[] = {
        "laying on of hands", "sacred weapon", "death blow",
        "crushing blow", "shield protection"
    };
    for (int i = 0; i < 5; ++i) {
        SDL_Texture* texture = i == 0 ? layingOnHandsTexture
            : (i == 1 ? sacredWeaponTexture
               : (i == 2 ? deathBlowTexture
                  : (i == 3 ? crushingBlowTexture
                            : shieldProtectionTexture)));
        if (!texture) continue;
        SDL_RenderCopy(renderer, texture, nullptr, &screenRect);
        const int cooldown =
            devotionSkills.cooldownRemaining(skillNames[i]);
        if (cooldown > 0) {
            const SDL_Rect hitbox = skillHitbox(i);
            SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
            SDL_SetRenderDrawColor(renderer, 0, 0, 0, 180);
            SDL_RenderFillRect(renderer, &hitbox);
            SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
            renderText(std::to_string(cooldown),
                       hitbox.x + hitbox.w / 3,
                       hitbox.y + hitbox.h / 5, textColor);
        }
    }
}

void TelaBattle::renderBerserkerSkills() {
    const SDL_Rect screenRect = {0, 0, screenW, screenH};
    const SDL_Color textColor = {255, 198, 1, 255};
    const char* skillNames[] = {"smash", "brutal cut", "fierce beat"};
    const char* labels[] = {"S", "BC", "FB"};
    SDL_Texture* textures[] = {
        smashTexture, brutalCutTexture, fierceBeatTexture
    };

    for (int i = 0; i < 3; ++i) {
        if (!berserkerSkills.isAvailable(player, skillNames[i])) continue;
        const SDL_Rect hitbox = skillHitbox(i);
        if (textures[i]) {
            SDL_RenderCopy(renderer, textures[i], nullptr, &screenRect);
        } else {
            SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
            SDL_SetRenderDrawColor(renderer, 55, 34, 12, 235);
            SDL_RenderFillRect(renderer, &hitbox);
            SDL_SetRenderDrawColor(renderer, 255, 198, 1, 255);
            SDL_RenderDrawRect(renderer, &hitbox);
            SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
            renderText(labels[i], hitbox.x + 3,
                       hitbox.y + (hitbox.h - TTF_FontHeight(uiFont)) / 2,
                       textColor);
        }
        const int cooldown =
            berserkerSkills.cooldownRemaining(skillNames[i]);
        if (cooldown > 0) {
            SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
            SDL_SetRenderDrawColor(renderer, 0, 0, 0, 180);
            SDL_RenderFillRect(renderer, &hitbox);
            SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
            renderText(std::to_string(cooldown),
                       hitbox.x + hitbox.w / 3,
                       hitbox.y + hitbox.h / 5, textColor);
        }
    }
}

void TelaBattle::renderBattleControls() {
    const SDL_Rect screenRect = {0, 0, screenW, screenH};
    if (infoButtonTexture) {
        SDL_RenderCopy(renderer, infoButtonTexture, nullptr, &screenRect);
    }
    if (bagButtonTexture) {
        SDL_RenderCopy(renderer, bagButtonTexture, nullptr, &screenRect);
    }
}

void TelaBattle::renderAttributesOverlay() {
    const SDL_Rect screenRect = {0, 0, screenW, screenH};
    SDL_RenderCopy(renderer, attributesTexture, nullptr, &screenRect);
    if (closeButtonTexture) {
        SDL_RenderCopy(renderer, closeButtonTexture, nullptr, &screenRect);
    }

    const int attributes[] = {
        player.strength, player.dexterity, player.constitution,
        player.intellect, player.wisdom, player.charisma
    };
    const char* labels[] = {"STR", "DEX", "CON", "INT", "WIS", "CHA"};
    const SDL_Color textColor = {255, 198, 1, 255};
    const int startY = screenH * 30 / 100;
    const int lineHeight = screenH * 38 / 1000;
    for (int i = 0; i < 6; ++i) {
        const int modifier = static_cast<int>(
            std::floor((attributes[i] - 10) / 2.0));
        const std::string modifierText =
            modifier >= 0 ? "(+ " + std::to_string(modifier) + ")"
                          : "(- " + std::to_string(std::abs(modifier)) + ")";
        renderCenteredText(
            std::string(labels[i]) + " " +
                std::to_string(attributes[i]) + " " + modifierText,
            screenW / 2,
            startY + i * lineHeight,
            textColor);
    }
}

void TelaBattle::renderInventoryOverlay() {
    const SDL_Rect screenRect = {0, 0, screenW, screenH};
    SDL_RenderCopy(renderer, inventoryTexture, nullptr, &screenRect);
    if (closeButtonTexture) {
        SDL_RenderCopy(renderer, closeButtonTexture, nullptr, &screenRect);
    }

    const SDL_Color headingColor = {255, 198, 1, 255};
    const SDL_Color textColor = {235, 235, 235, 255};
    const SDL_Color emptyColor = {160, 160, 160, 255};
    const int rowHeight = std::max(24, screenH * 38 / 1000);
    const int startY = screenH * 19 / 100;
    renderText("ITEM", screenW * 8 / 100, startY, headingColor);
    renderText("QTY", screenW * 55 / 100, startY, headingColor);
    renderText("WEIGHT", screenW * 70 / 100, startY, headingColor);
    renderText("STATE", screenW * 83 / 100, startY, headingColor);

    const std::size_t visibleRows = 18;
    const std::size_t endIndex = std::min(
        player.inventory.size(), inventoryScrollOffset + visibleRows);
    int y = startY + rowHeight;
    for (std::size_t i = inventoryScrollOffset; i < endIndex; ++i) {
        const player_save::BagItem& item = player.inventory[i];
        std::ostringstream weightText;
        weightText << std::fixed << std::setprecision(2)
                   << item.weight * item.quantity;
        const SDL_Color itemColor =
            item.state == "vazio" ? emptyColor : textColor;
        renderText(item.name, screenW * 8 / 100, y, itemColor);
        renderText(std::to_string(item.quantity),
                   screenW * 55 / 100, y, textColor);
        renderText(weightText.str(), screenW * 70 / 100, y, textColor);
        renderText(item.state.empty() ? "-" : item.state,
                   screenW * 83 / 100, y, textColor);
        y += rowHeight;
    }
    if (player.inventory.empty()) {
        renderCenteredText("O inventario esta vazio.", screenW / 2,
                           startY + rowHeight * 2, emptyColor);
    } else if (player.inventory.size() > visibleRows) {
        renderCenteredText("Use a roda do mouse para ver os outros itens.",
                           screenW / 2, screenH * 92 / 100, emptyColor);
    }
}

void TelaBattle::toggleAttributesOverlay() {
    attributesOpen = !attributesOpen;
    if (attributesOpen) inventoryOpen = false;
}

void TelaBattle::toggleInventoryOverlay() {
    inventoryOpen = !inventoryOpen;
    if (inventoryOpen) {
        attributesOpen = false;
        inventoryScrollOffset = 0;
    }
}

std::vector<std::string> TelaBattle::wrappedBattleLogLines() const {
    const int maxWidth = screenW * 350 / 1400;
    std::vector<std::string> lines;

    for (const std::string& entry : battleLogs) {
        std::string current;
        std::istringstream words(entry);
        std::string word;
        while (words >> word) {
            const std::string candidate =
                current.empty() ? word : current + " " + word;
            int textWidth = 0;
            if (uiFont &&
                TTF_SizeUTF8(uiFont, candidate.c_str(),
                             &textWidth, nullptr) == 0 &&
                textWidth > maxWidth && !current.empty()) {
                lines.push_back(current);
                current = word;
            } else {
                current = candidate;
            }
        }
        if (!current.empty()) {
            lines.push_back(current);
        }
    }
    return lines;
}

void TelaBattle::renderBattleLog() {
    const int x = screenW * 1030 / 1400;
    const int y = screenH * 606 / 850;
    const int lineHeight = std::max(18, screenH * 28 / 850);
    const std::size_t maxLines = 7;
    std::vector<std::string> lines = wrappedBattleLogLines();
    const std::size_t maxOffset =
        lines.size() > maxLines ? lines.size() - maxLines : 0;
    battleLogScrollOffset = std::min(battleLogScrollOffset, maxOffset);
    const std::size_t end =
        lines.size() - battleLogScrollOffset;
    const std::size_t start =
        end > maxLines ? end - maxLines : 0;
    const SDL_Color textColor = {255, 198, 1, 255};
    for (std::size_t i = start; i < end; ++i) {
        renderText(lines[i], x,
                   y + static_cast<int>(i - start) * lineHeight,
                   textColor);
    }
    if (maxOffset > 0) {
        renderText("Roda do mouse: rolar (" +
                       std::to_string(maxOffset - battleLogScrollOffset + 1) +
                       "/" + std::to_string(maxOffset + 1) + ")",
                   x, y + static_cast<int>(maxLines) * lineHeight,
                   textColor);
    }
}
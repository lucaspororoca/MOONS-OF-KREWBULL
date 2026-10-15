#include "scripts/screens/screen_game.h"
#include "scripts/resources/player_save.h"
#include "scripts/resources/resource_archive.h"
#include "scripts/hud/bgm_manager.h"
#include "scripts/skills/paladino_devotion.h"
#include "scripts/skills/barbarian_berserker.h"
#include "scripts/wearpons/iron_shield.h"
#include "scripts/armors/plate_armor.h"
#include "scripts/characters/ogro.h"
#include <algorithm>
#include <iostream>
#include <string>

TelaGame::TelaGame(SDL_Renderer* renderer, int screenW, int screenH,
                   const std::filesystem::path& savePath)
    : renderer(renderer),
      screenW(screenW),
      screenH(screenH),
      running(true),
      requestedSavePath(savePath),
      saveTimer(0.0f),
      battleRequested(false),
      battleTriggered(false),
      battleEnemies(),
      battleStartX(0.0f),
      battleStartY(0.0f),
      preBattleCharacter(),
      hasPreBattleSnapshot(false),
      progressionFont(nullptr),
      player(renderer,
             12 * CavesOfChaos::TILE_SIZE + CavesOfChaos::TILE_SIZE / 2.0f,
              7 * CavesOfChaos::TILE_SIZE + CavesOfChaos::TILE_SIZE / 2.0f),
      camera(screenW, screenH)
{
}

TelaGame::~TelaGame() {
    map.saveWorldState(player);
    if (progressionFont) TTF_CloseFont(progressionFont);
}

bool TelaGame::init() {
    player_save::Character savedCharacter;
    std::filesystem::path savePath;
    std::string saveError;
    bool loaded = false;
    if (requestedSavePath.empty()) {
        loaded = player_save::readLatest(savePath, savedCharacter, saveError);
    } else {
        savePath = requestedSavePath;
        loaded = player_save::readFile(savePath, savedCharacter, saveError);
    }
    if (!loaded) {
        std::cerr << "Falha ao carregar o personagem salvo: "
                  << saveError << std::endl;
        return false;
    }

    player.name = savedCharacter.name;
    player.characterClass = savedCharacter.cls;
    player.subclass = savedCharacter.subclass;
    player.race = savedCharacter.race;
    player.subrace = savedCharacter.subrace;
    player.gender = savedCharacter.gender;
    player.skills = savedCharacter.skills;
    player.inventory = savedCharacter.bag;
    player.experience = savedCharacter.experience;
    bool inventoryMigrated = false;
    if (player.characterClass == "paladin" ||
        player.characterClass == "paladino") {
        const auto armor = std::find_if(
            player.inventory.begin(), player.inventory.end(),
            [](const player_save::BagItem& item) {
                return item.name == PlateArmor::name() &&
                       item.quantity > 0;
            });
        if (armor == player.inventory.end()) {
            player.inventory.push_back(
                {PlateArmor::name(), 1, PlateArmor::weight(), "equipado"});
            inventoryMigrated = true;
        } else {
            if (armor->state != "equipado") {
                armor->state = "equipado";
                inventoryMigrated = true;
            }
        }
    }
    if (inventoryMigrated) {
        std::string inventoryError;
        if (!player_save::writeBag(savePath, player.inventory, inventoryError)) {
            std::cerr << "Falha ao atualizar inventario do personagem: "
                      << inventoryError << std::endl;
        }
    }
    bool skillsMigrated = false;
    if ((player.characterClass == "paladin" ||
         player.characterClass == "paladino") &&
        player.subclass == "oath_devotion") {
        std::vector<player_save::Skill> devotionSkills;
        paladino_devotion::populateSkills(
            player.characterClass, player.subclass, devotionSkills);
        for (const player_save::Skill& devotionSkill : devotionSkills) {
            const auto savedSkill = std::find_if(
                player.skills.begin(), player.skills.end(),
                [&devotionSkill](const player_save::Skill& savedSkill) {
                    return savedSkill.name == devotionSkill.name;
                });
            if (savedSkill == player.skills.end()) {
                player.skills.push_back(devotionSkill);
                skillsMigrated = true;
                continue;
            }
            if (savedSkill->cooldown <= 0) {
                savedSkill->cooldown = devotionSkill.cooldown;
                skillsMigrated = true;
            }
            if (devotionSkill.name == "laying on of hands" &&
                (savedSkill->description != devotionSkill.description ||
                 savedSkill->effect != devotionSkill.effect)) {
                savedSkill->description = devotionSkill.description;
                savedSkill->effect = devotionSkill.effect;
                skillsMigrated = true;
            }
        }
    }
    if ((player.characterClass == "barbarian" ||
         player.characterClass == "barbaro") &&
        (player.subclass == "path_berserker" ||
         player.subclass == "berserker")) {
        std::vector<player_save::Skill> berserkerSkills;
        barbarian_berserker::populateSkills(
            player.characterClass, player.subclass, berserkerSkills);
        for (const player_save::Skill& berserkerSkill : berserkerSkills) {
            const auto savedSkill = std::find_if(
                player.skills.begin(), player.skills.end(),
                [&berserkerSkill](const player_save::Skill& savedSkill) {
                    return savedSkill.name == berserkerSkill.name;
                });
            if (savedSkill == player.skills.end()) {
                player.skills.push_back(berserkerSkill);
                skillsMigrated = true;
                continue;
            }
            if (savedSkill->cooldown <= 0) {
                savedSkill->cooldown = berserkerSkill.cooldown;
                skillsMigrated = true;
            }
            if (savedSkill->description != berserkerSkill.description ||
                savedSkill->effect != berserkerSkill.effect) {
                savedSkill->description = berserkerSkill.description;
                savedSkill->effect = berserkerSkill.effect;
                skillsMigrated = true;
            }
        }
    }
    if (skillsMigrated) {
        std::string skillsError;
        if (!player_save::writeSkills(savePath, player.skills, skillsError)) {
            std::cerr << "Falha ao atualizar skills do personagem: "
                      << skillsError << std::endl;
        }
    }
    const std::string savedSpritePath = savedCharacter.spritePath.empty()
        ? player_save::spriteDirectory(
              savedCharacter.cls, savedCharacter.race,
              savedCharacter.subrace, savedCharacter.gender)
        : savedCharacter.spritePath;
    if (!savedSpritePath.empty() &&
        !player.setSpriteDirectory(renderer, savedSpritePath)) {
        player.spritePath = savedSpritePath;
    }
    player.level = std::max(1, std::min(savedCharacter.level, Player::MAX_LEVEL));
    player.strength = savedCharacter.attributes.strength;
    player.dexterity = savedCharacter.attributes.dexterity;
    player.constitution = savedCharacter.attributes.constitution;
    player.intellect = savedCharacter.attributes.intellect;
    player.wisdom = savedCharacter.attributes.wisdom;
    player.charisma = savedCharacter.attributes.charisma;
    if (savedCharacter.maxHp > 0 &&
        savedCharacter.hpFormulaVersion >= 1) {
        player.maxHp = savedCharacter.maxHp;
        player.hp = std::max(0, std::min(savedCharacter.hp, player.maxHp));
    } else {
        player.initializeHitPoints();
        std::string vitalsError;
        if (!player_save::writeVitals(
                savePath, player.hp, player.maxHp, vitalsError)) {
            std::cerr << "Falha ao salvar pontos de vida iniciais: "
                      << vitalsError << std::endl;
        }
    }
    player.armorClass = IronShield::armorClassFor(player);
    if (savedCharacter.hasWorldState) {
        player.setPosition(savedCharacter.playerPosition.x,
                           savedCharacter.playerPosition.y);
    }
    progressionFont = resource_archive::openResourceFont(
        "resources/fonts/impact.ttf", 22);
    if (!progressionFont) {
        std::cerr << "Falha ao carregar a fonte de progressao: "
                  << TTF_GetError() << std::endl;
        return false;
    }
    map.setPlayerSavePath(savePath);

    if (!map.load(renderer)) {
        std::cerr << "Falha ao carregar o mapa!" << std::endl;
        return false;
    }

    map.saveWorldState(player);
    camera.setBounds(map.getPixelWidth(), map.getPixelHeight());
    return true;
}

void TelaGame::handleEvent(const SDL_Event& event) {
    if (event.type == SDL_QUIT) {
        map.saveWorldState(player);
        running = false;
        return;
    }
    if (event.type == SDL_WINDOWEVENT &&
        event.window.event == SDL_WINDOWEVENT_FOCUS_LOST) {
        player.clearMovementInput();
    }
    player.handleEvent(event);
    map.handleEvent(event, player);
    player.armorClass = IronShield::armorClassFor(player);
}

void TelaGame::update(float dt) {
    player.update(dt, map);

    float hx, hy; int hw, hh;
    player.getHitbox(hx, hy, hw, hh);
    map.update(dt, hx, hy, hw, hh, player);
    player.armorClass = IronShield::armorClassFor(player);

    player.getHitbox(hx, hy, hw, hh);
    std::vector<CombatEnemy*> touchingEnemies =
        map.enemiesTouching(hx, hy, hw, hh);
    if (touchingEnemies.empty()) {
        battleTriggered = false;
    } else if (!battleTriggered) {
        battleTriggered = true;
        battleRequested = true;
        battleStartX = player.getX();
        battleStartY = player.getY();
        preBattleCharacter.name = player.name;
        preBattleCharacter.cls = player.characterClass;
        preBattleCharacter.subclass = player.subclass;
        preBattleCharacter.race = player.race;
        preBattleCharacter.subrace = player.subrace;
        preBattleCharacter.gender = player.gender;
        preBattleCharacter.spritePath = player.spritePath;
        preBattleCharacter.level = player.level;
        preBattleCharacter.experience = player.experience;
        preBattleCharacter.hp = player.hp;
        preBattleCharacter.maxHp = player.maxHp;
        preBattleCharacter.attributes.strength = player.strength;
        preBattleCharacter.attributes.dexterity = player.dexterity;
        preBattleCharacter.attributes.constitution = player.constitution;
        preBattleCharacter.attributes.intellect = player.intellect;
        preBattleCharacter.attributes.wisdom = player.wisdom;
        preBattleCharacter.attributes.charisma = player.charisma;
        preBattleCharacter.bag = player.inventory;
        preBattleCharacter.skills = player.skills;
        preBattleCharacter.playerPosition.x = battleStartX;
        preBattleCharacter.playerPosition.y = battleStartY;
        hasPreBattleSnapshot = true;
        player.clearMovementInput();
        battleEnemies.assign(1, touchingEnemies.front());
        map.saveWorldState(player);
    }

    saveTimer += dt;
    if (saveTimer >= 1.0f) {
        saveTimer = 0.0f;
        map.saveWorldState(player);
    }

    camera.follow(player.getX(), player.getY());
}

std::vector<CombatEnemy*> TelaGame::consumeBattleRequest() {
    if (!battleRequested) return {};
    battleRequested = false;
    return battleEnemies;
}

void TelaGame::resumeAfterBattle() {
    player.setPosition(battleStartX, battleStartY);
    player.clearMovementInput();
    map.removeDefeatedEnemies();
    map.saveWorldState(player);
    battleEnemies.clear();
    battleTriggered = false;
    bgm_manager::playLoop("resources/audio/bgm/maps/caves_of_chaos.mp3");
}

void TelaGame::resumeAfterGameOver() {
    if (hasPreBattleSnapshot) {
        player.name = preBattleCharacter.name;
        player.characterClass = preBattleCharacter.cls;
        player.subclass = preBattleCharacter.subclass;
        player.race = preBattleCharacter.race;
        player.subrace = preBattleCharacter.subrace;
        player.gender = preBattleCharacter.gender;
        player.spritePath = preBattleCharacter.spritePath;
        player.level = preBattleCharacter.level;
        player.experience = preBattleCharacter.experience;
        player.maxHp = std::max(1, preBattleCharacter.maxHp);
        player.hp = player.maxHp;
        player.strength = preBattleCharacter.attributes.strength;
        player.dexterity = preBattleCharacter.attributes.dexterity;
        player.constitution = preBattleCharacter.attributes.constitution;
        player.intellect = preBattleCharacter.attributes.intellect;
        player.wisdom = preBattleCharacter.attributes.wisdom;
        player.charisma = preBattleCharacter.attributes.charisma;
        player.inventory = preBattleCharacter.bag;
        player.skills = preBattleCharacter.skills;
        player.setPosition(battleStartX, battleStartY);
        player.armorClass = IronShield::armorClassFor(player);
    } else {
        player.hp = player.maxHp;
    }
    player.clearMovementInput();
    battleEnemies.clear();
    battleTriggered = false;
    battleRequested = false;
    map.saveWorldState(player);
    bgm_manager::playLoop("resources/audio/bgm/maps/caves_of_chaos.mp3");
}

void TelaGame::render() {
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);

    map.render(renderer, camera.getX(), camera.getY(), &player);
    map.renderOverlay(renderer);

    if (progressionFont) {
        const SDL_Color color = {255, 198, 1, 255};
        const std::string levelText =
            "Level: " + std::to_string(player.level);
        const int xpRequired = player.experienceToNextLevel();
        const std::string xpText = xpRequired > 0
            ? "XP: " + std::to_string(player.experience) + "/" +
                  std::to_string(xpRequired)
            : "XP: MAX LEVEL";
        const std::string lines[] = {levelText, xpText};
        for (int i = 0; i < 2; ++i) {
            SDL_Surface* surface = TTF_RenderUTF8_Blended(
                progressionFont, lines[i].c_str(), color);
            if (!surface) {
                std::cerr << "Falha ao renderizar progresso do jogador: "
                          << TTF_GetError() << std::endl;
                continue;
            }
            SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
            if (!texture) {
                std::cerr << "Falha ao criar texto de progresso: "
                          << SDL_GetError() << std::endl;
                SDL_FreeSurface(surface);
                continue;
            }
            const SDL_Rect destination = {
                screenW * 18 / 1400,
                screenH * 770 / 850 + i * TTF_FontLineSkip(progressionFont),
                surface->w, surface->h};
            SDL_RenderCopy(renderer, texture, nullptr, &destination);
            SDL_DestroyTexture(texture);
            SDL_FreeSurface(surface);
        }
    }
}
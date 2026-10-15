#include "scripts/maps/caves_of_chaos.h"
#include "scripts/characters/player.h"
#include "scripts/characters/ogro.h"
#include "scripts/characters/rat.h"
#include "scripts/resources/resource_archive.h"
#include "scripts/resources/player_save.h"
#include "scripts/hud/bgm_manager.h"
#include <algorithm>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <sstream>

CavesOfChaos::CavesOfChaos()
    : rows(0), cols(0),
      bgm(nullptr), audioOpen(false),
      activeDoorRow(-1), activeDoorCol(-1),
      activeTablePaperRow(-1), activeTablePaperCol(-1),
      activeGroundIdx(-1),
      paperIconTex(nullptr), paperIconW(0), paperIconH(0),
      potionIconTex(nullptr), potionIconW(0), potionIconH(0),
      noticeCollectTex(nullptr), noticeCollectW(0), noticeCollectH(0),
      activePotionRow(-1), activePotionCol(-1),
      collectTextboxTex(nullptr), collectTextboxW(0), collectTextboxH(0),
      renderer(nullptr),
      ogroMaxCount(20),
      ogroRespawnTime(600.0f),
      ogroRespawnTimer(0.0f),
      ratMaxCount(5),
      ratRespawnTime(180.0f),
      ratRespawnTimer(0.0f)
{
    loadTilesFromJson();

    rows = static_cast<int>(tiles.size());
    cols = rows > 0 ? static_cast<int>(tiles[0].size()) : 0;

    ogroSpawnPoints = {
        {  9, 10 },
        { 10, 15 },
        {  6, 18 },
        { 15, 22 },
        { 21, 22 },
        { 18, 8 },
        { 14, 11 },
        {  5, 25 },
        {  8, 30 },
        { 12, 35 },
        { 20, 40 },
        { 25, 45 },
        { 30, 50 },
        { 35, 55 },
        { 40, 60 },
        { 45, 65 },
        { 50, 70 },
        { 55, 75 },
        { 60, 80 },
        { 65, 85 },
    };
    ratSpawnPoints = {
        {  3,  3 },
        {  7, 20 },
        { 13, 15 },
        { 18, 35 },
        { 24, 11 },
    };

}

void CavesOfChaos::loadTilesFromJson() {
    tiles.clear();

    std::string content;
    std::string errorMessage;
    if (!resource_archive::readResourceText(
            "resources/data/maps/map_cave_of_chaos.json", content, errorMessage)) {
        std::cerr << "Falha ao abrir resources/data/map_cave_of_chaos.json"
                  << ": " << errorMessage
                  << std::endl;
        return;
    }

    size_t tilesKey = content.find("\"tiles\"");
    if (tilesKey == std::string::npos) {
        std::cerr << "Chave \"tiles\" nao encontrada no JSON" << std::endl;
        return;
    }

    size_t arrStart = content.find('[', tilesKey);
    size_t arrEnd   = content.find(']', arrStart);
    if (arrStart == std::string::npos || arrEnd == std::string::npos) {
        std::cerr << "Array de tiles invalido no JSON" << std::endl;
        return;
    }

    size_t pos = arrStart;
    while (true) {
        size_t openQuote = content.find('"', pos + 1);
        if (openQuote == std::string::npos || openQuote >= arrEnd) break;

        size_t closeQuote = content.find('"', openQuote + 1);
        if (closeQuote == std::string::npos || closeQuote > arrEnd) break;

        std::string line = content.substr(openQuote + 1,
                                          closeQuote - openQuote - 1);
        tiles.push_back(line);

        pos = closeQuote;
    }

    if (tiles.empty()) {
        std::cerr << "Nenhum tile carregado do JSON" << std::endl;
    } else {
        std::cout << "Mapa carregado: " << tiles.size() << " linhas, "
                  << tiles[0].size() << " colunas" << std::endl;
    }
}

CavesOfChaos::~CavesOfChaos() {
    if (bgm) {
        Mix_HaltMusic();
        Mix_FreeMusic(bgm);
        bgm = nullptr;
    }
    if (audioOpen) {
        Mix_CloseAudio();
        audioOpen = false;
    }

    if (paperIconTex)      SDL_DestroyTexture(paperIconTex);
    if (potionIconTex)     SDL_DestroyTexture(potionIconTex);
    if (noticeCollectTex)  SDL_DestroyTexture(noticeCollectTex);
    if (collectTextboxTex) SDL_DestroyTexture(collectTextboxTex);
}

bool CavesOfChaos::load(SDL_Renderer* renderer) {
    this->renderer = renderer;

    if (!bag.load()) {
        std::cerr << "Falha ao carregar dados da bag do save ativo." << std::endl;
        return false;
    }

    if (!floor.load(renderer)) {
        std::cerr << "Falha ao carregar sprites de floor!" << std::endl;
        return false;
    }
    if (!wall.load(renderer)) {
        std::cerr << "Falha ao carregar sprites de wall!" << std::endl;
        return false;
    }
    if (!door.load(renderer)) {
        std::cerr << "Falha ao carregar sprites de door!" << std::endl;
        return false;
    }
    if (!tableEmpty.load(renderer)) {
        std::cerr << "Falha ao carregar sprites de table empty!" << std::endl;
        return false;
    }
    if (!tablePaper.load(renderer)) {
        std::cerr << "Falha ao carregar sprites de table paper!" << std::endl;
        return false;
    }
    if (!chair.load(renderer)) {
        std::cerr << "Falha ao carregar sprites de Chair" << std::endl;
        return false;
    }
    if (!boockcase.load(renderer)) {
        std::cerr << "Falha ao carregar sprites do boockcase." << std::endl;
        return false;
    }
    if (!bag.loadUI(renderer)) {
        std::cerr << "Falha ao carregar UI do Bag!" << std::endl;
        return false;
    }
    if (!potionItem.load(renderer)) {
        std::cerr << "Falha ao carregar Potion HP Minor!" << std::endl;
        return false;
    }

    {
        SDL_Surface* iconSurf = resource_archive::loadResourceImage(
            "resources/sprites/objects/icon/icon_paper.png");
        if (iconSurf) {
            paperIconTex = SDL_CreateTextureFromSurface(renderer, iconSurf);
            paperIconW = iconSurf->w;
            paperIconH = iconSurf->h;
            SDL_FreeSurface(iconSurf);
            SDL_SetTextureScaleMode(paperIconTex, SDL_ScaleModeNearest);
        } else {
            std::cerr << "Falha ao carregar icon_paper.png: "
                      << IMG_GetError() << std::endl;
        }
    }

    {
        SDL_Surface* potionSurf = resource_archive::loadResourceImage(
            "resources/sprites/objects/icon/icon_potion_hp_minor.png");
        if (potionSurf) {
            potionIconTex = SDL_CreateTextureFromSurface(renderer, potionSurf);
            potionIconW = potionSurf->w;
            potionIconH = potionSurf->h;
            SDL_FreeSurface(potionSurf);
            SDL_SetTextureScaleMode(potionIconTex, SDL_ScaleModeNearest);
        } else {
            std::cerr << "Falha ao carregar icon_potion_hp_minor.png: "
                      << IMG_GetError() << std::endl;
        }
    }

    {
        SDL_Surface* noticeSurf = resource_archive::loadResourceImage(
            "resources/interfaces/notices/notice_option_collect.png");
        if (noticeSurf) {
            noticeCollectTex = SDL_CreateTextureFromSurface(renderer, noticeSurf);
            noticeCollectW = noticeSurf->w;
            noticeCollectH = noticeSurf->h;
            SDL_FreeSurface(noticeSurf);
            SDL_SetTextureScaleMode(noticeCollectTex, SDL_ScaleModeNearest);
        } else {
            std::cerr << "Falha ao carregar notice_option_collect.png: "
                      << IMG_GetError() << std::endl;
        }
    }

    {
        SDL_Surface* collectSurf = resource_archive::loadResourceImage(
            "resources/interfaces/notices/notice_option_collect.png");
        if (collectSurf) {
            collectTextboxTex = SDL_CreateTextureFromSurface(renderer, collectSurf);
            collectTextboxW = collectSurf->w;
            collectTextboxH = collectSurf->h;
            SDL_FreeSurface(collectSurf);
            SDL_SetTextureScaleMode(collectTextboxTex, SDL_ScaleModeNearest);
        } else {
            std::cerr << "Falha ao carregar textbox_collect.png: "
                      << IMG_GetError() << std::endl;
        }
    }

    loadSavedDroppedItems();
    if (door.applySavedStates(tiles)) {
        saveMapTiles();
    }
    bgm_manager::playLoop("resources/audio/bgm/maps/caves_of_chaos.mp3");

    spawnInitialEnemies();

    return true;
}

void CavesOfChaos::spawnOgro() {
    if (!renderer) return;
    if (ogroSpawnPoints.empty()) return;

    int idx = std::rand() % ogroSpawnPoints.size();
    int row = ogroSpawnPoints[idx].first;
    int col = ogroSpawnPoints[idx].second;

    float sx = col * TILE_SIZE + TILE_SIZE / 2.0f;
    float sy = row * TILE_SIZE + TILE_SIZE / 2.0f;

    spawnOgro(sx, sy);
}

void CavesOfChaos::spawnOgro(float x, float y) {
    if (!renderer) return;
    ogros.push_back(std::unique_ptr<Ogro>(new Ogro(renderer, x, y)));
}

void CavesOfChaos::spawnRat() {
    if (!renderer || ratSpawnPoints.empty()) return;
    const std::size_t index =
        static_cast<std::size_t>(std::rand()) % ratSpawnPoints.size();
    const int row = ratSpawnPoints[index].first;
    const int col = ratSpawnPoints[index].second;
    spawnRat(col * TILE_SIZE + TILE_SIZE / 2.0f,
             row * TILE_SIZE + TILE_SIZE / 2.0f);
}

void CavesOfChaos::spawnRat(float x, float y) {
    if (!renderer) return;
    rats.push_back(std::unique_ptr<Rat>(new Rat(renderer, x, y)));
}

void CavesOfChaos::spawnInitialEnemies() {
    bool loadedSavedEnemies = false;
    if (!playerSavePath.empty()) {
        player_save::Character character;
        std::string error;
        if (!player_save::readFile(playerSavePath, character, error)) {
            std::cerr << "Falha ao carregar posicoes dos inimigos: "
                      << error << std::endl;
        } else if (character.hasWorldState) {
            for (const player_save::Position& position :
                 character.enemyPositions) {
                spawnOgro(position.x, position.y);
            }
            if (character.hasRatPositions) {
                for (const player_save::Position& position :
                     character.ratEnemyPositions) {
                    spawnRat(position.x, position.y);
                }
            } else {
                for (int i = 0; i < ratMaxCount; ++i) spawnRat();
            }
            loadedSavedEnemies = true;
        }
    }
    if (!loadedSavedEnemies) {
        for (int i = 0; i < ogroMaxCount; ++i) spawnOgro();
        for (int i = 0; i < ratMaxCount; ++i) spawnRat();
    }
}

void CavesOfChaos::saveWorldState(const Player& player) const {
    if (playerSavePath.empty()) return;
    std::string error;
    player_save::Position playerPosition;
    playerPosition.x = player.getX();
    playerPosition.y = player.getY();

    std::vector<player_save::Position> enemyPositions;
    std::vector<player_save::Position> ratPositions;
    enemyPositions.reserve(ogros.size());
    ratPositions.reserve(rats.size());
    for (const std::unique_ptr<Ogro>& ogro : ogros) {
        if (!ogro || !ogro->isAlive()) continue;
        player_save::Position position;
        position.x = ogro->getX();
        position.y = ogro->getY();
        enemyPositions.push_back(position);
    }
    for (const std::unique_ptr<Rat>& rat : rats) {
        if (!rat || !rat->isAlive()) continue;
        player_save::Position position;
        position.x = rat->getX();
        position.y = rat->getY();
        ratPositions.push_back(position);
    }
    if (!player_save::writeWorldState(
            playerSavePath, playerPosition, enemyPositions, ratPositions,
            player.hp, player.maxHp, player.level, player.experience, error)) {
        std::cerr << "Falha ao salvar posicoes do mundo: " << error
                  << std::endl;
    }
}

void CavesOfChaos::updateEnemies(float dt, const Player& player) {
    for (auto& o : ogros) {
        if (o && o->isAlive()) {
            o->update(dt, *this, player);
        }
    }
    for (auto& rat : rats) {
        if (rat && rat->isAlive()) rat->update(dt, *this, player);
    }

    removeDefeatedEnemies();

    if (static_cast<int>(ogros.size()) < ogroMaxCount) {
        ogroRespawnTimer += dt;
        if (ogroRespawnTimer >= ogroRespawnTime) {
            ogroRespawnTimer = 0.0f;
            spawnOgro();
        }
    } else {
        ogroRespawnTimer = 0.0f;
    }

    if (static_cast<int>(rats.size()) < ratMaxCount) {
        ratRespawnTimer += dt;
        if (ratRespawnTimer >= ratRespawnTime) {
            ratRespawnTimer = 0.0f;
            spawnRat();
        }
    } else {
        ratRespawnTimer = 0.0f;
    }
}

void CavesOfChaos::spawnDroppedItem(const std::string& name, float weight,
                                    float x, float y,
                                    SDL_Texture* iconTex, int iconW, int iconH)
{
    DroppedItem item;
    item.name    = name;
    item.weight  = weight;
    item.x       = x;
    item.y       = y;
    item.iconTex = iconTex;
    item.iconW   = iconW;
    item.iconH   = iconH;
    droppedItems.push_back(item);
    saveDroppedItems();
}

void CavesOfChaos::loadSavedDroppedItems() {
    if (playerSavePath.empty()) return;

    player_save::Character character;
    std::string error;
    if (!player_save::readFile(playerSavePath, character, error)) {
        std::cerr << error << std::endl;
        return;
    }

    bool validMapTiles = character.mapTiles.size() == tiles.size();
    for (std::size_t i = 0; validMapTiles && i < tiles.size(); ++i) {
        validMapTiles = character.mapTiles[i].size() == tiles[i].size();
    }
    if (validMapTiles) {
        tiles = character.mapTiles;
    } else {
        saveMapTiles();
    }

    droppedItems.clear();
    for (std::size_t i = 0; i < character.items.size(); ++i) {
        const player_save::GroundItem& saved = character.items[i];
        SDL_Texture* icon = nullptr;
        int iconW = 0;
        int iconH = 0;
        if (saved.name == "Paper") {
            icon = paperIconTex;
            iconW = paperIconW;
            iconH = paperIconH;
        } else if (saved.name == "Potion HP minor") {
            icon = potionIconTex;
            iconW = potionIconW;
            iconH = potionIconH;
        }
        if (!icon) {
            std::cerr << "Item salvo sem icone de mapa suportado: "
                      << saved.name << std::endl;
            continue;
        }
        DroppedItem item;
        item.name = saved.name;
        item.weight = saved.weight;
        item.x = saved.x;
        item.y = saved.y;
        item.iconTex = icon;
        item.iconW = iconW;
        item.iconH = iconH;
        droppedItems.push_back(item);
    }
}

void CavesOfChaos::saveDroppedItems() const {
    if (playerSavePath.empty()) return;

    std::vector<player_save::GroundItem> items;
    for (std::size_t i = 0; i < droppedItems.size(); ++i) {
        player_save::GroundItem item;
        item.name = droppedItems[i].name;
        item.weight = droppedItems[i].weight;
        item.x = droppedItems[i].x;
        item.y = droppedItems[i].y;
        items.push_back(item);
    }
    std::string error;
    if (!player_save::writeItems(playerSavePath, items, error)) {
        std::cerr << error << std::endl;
    }
}

void CavesOfChaos::saveMapTiles() const {
    if (playerSavePath.empty()) return;
    std::string error;
    if (!player_save::writeMapTiles(playerSavePath, tiles, error)) {
        std::cerr << error << std::endl;
    }
}

void CavesOfChaos::updateDroppedItems(float px, float py, int pw, int ph) {
    activeGroundIdx = -1;

    for (size_t i = 0; i < droppedItems.size(); ++i) {
        if (!droppedItems[i].iconTex) continue;

        float ix = droppedItems[i].x - droppedItems[i].iconW / 2.0f;
        float iy = droppedItems[i].y - droppedItems[i].iconH / 2.0f;

        if (px < ix + droppedItems[i].iconW && px + pw > ix &&
            py < iy + droppedItems[i].iconH && py + ph > iy) {
            activeGroundIdx = static_cast<int>(i);
            break;
        }
    }
}

void CavesOfChaos::renderDroppedItems(SDL_Renderer* renderer,
                                      int camX, int camY)
{
    for (size_t i = 0; i < droppedItems.size(); ++i) {
        if (!droppedItems[i].iconTex) continue;

        SDL_Rect dst;
        dst.w = droppedItems[i].iconW;
        dst.h = droppedItems[i].iconH;
        dst.x = static_cast<int>(droppedItems[i].x) - droppedItems[i].iconW / 2 - camX;
        dst.y = static_cast<int>(droppedItems[i].y) - droppedItems[i].iconH / 2 - camY;
        SDL_RenderCopy(renderer, droppedItems[i].iconTex, nullptr, &dst);
    }
}

void CavesOfChaos::renderGroundPrompt(SDL_Renderer* renderer,
                                      int camX, int camY,
                                      const Player& player)
{
    if (!noticeCollectTex) return;

    const int GAP = 10;
    const int PLAYER_SPRITE = Player::SPRITE_SIZE;

    int playerScreenX = static_cast<int>(player.getX()) - camX;
    int playerScreenY = static_cast<int>(player.getY()) - camY;

    SDL_Rect dst;
    dst.w = noticeCollectW;
    dst.h = noticeCollectH;
    dst.x = playerScreenX + PLAYER_SPRITE / 2 + GAP;
    dst.y = playerScreenY - noticeCollectH / 2;

    SDL_RenderCopy(renderer, noticeCollectTex, nullptr, &dst);
}

void CavesOfChaos::updatePotionCollision(float px, float py, int pw, int ph) {
    activePotionRow = -1;
    activePotionCol = -1;

    int left   = static_cast<int>(px) / TILE_SIZE;
    int top    = static_cast<int>(py) / TILE_SIZE;
    int right  = static_cast<int>((px + pw - 1)) / TILE_SIZE;
    int bottom = static_cast<int>((py + ph - 1)) / TILE_SIZE;

    if (left < 0) left = 0;
    if (top < 0) top = 0;
    if (right >= cols) right = cols - 1;
    if (bottom >= rows) bottom = rows - 1;

    for (int ty = top; ty <= bottom; ++ty) {
        for (int tx = left; tx <= right; ++tx) {
            if (tiles[ty][tx] == 'p') {
                activePotionRow = ty;
                activePotionCol = tx;
                return;
            }
        }
    }
}

void CavesOfChaos::renderPotionTiles(SDL_Renderer* renderer,
                                     int camX, int camY)
{
    int startCol = camX / TILE_SIZE;
    int startRow = camY / TILE_SIZE;
    int endCol   = (camX + 1400) / TILE_SIZE + 1;
    int endRow   = (camY + 850) / TILE_SIZE + 1;

    if (startCol < 0) startCol = 0;
    if (startRow < 0) startRow = 0;
    if (endCol > cols) endCol = cols;
    if (endRow > rows) endRow = rows;

    for (int y = startRow; y < endRow; ++y) {
        for (int x = startCol; x < endCol; ++x) {
            if (tiles[y][x] == 'p') {
                int px = x * TILE_SIZE - camX;
                int py = y * TILE_SIZE - camY;
                potionItem.renderSpriteScaled(renderer, px, py,
                                              TILE_SIZE, TILE_SIZE);
            }
        }
    }
}

void CavesOfChaos::renderPotionPrompt(SDL_Renderer* renderer,
                                      int camX, int camY,
                                      const Player& player)
{
    if (!collectTextboxTex) return;

    const int GAP = 10;
    const int PLAYER_SPRITE = Player::SPRITE_SIZE;

    int playerScreenX = static_cast<int>(player.getX()) - camX;
    int playerScreenY = static_cast<int>(player.getY()) - camY;

    SDL_Rect dst;
    dst.w = collectTextboxW;
    dst.h = collectTextboxH;
    dst.x = playerScreenX + PLAYER_SPRITE / 2 + GAP;
    dst.y = playerScreenY - collectTextboxH / 2;

    SDL_RenderCopy(renderer, collectTextboxTex, nullptr, &dst);
}

void CavesOfChaos::update(float dt, float px, float py, int pw, int ph, const Player& player) {
    bag.updateUI(dt);

    if (bag.isOpen()) return;

    if (door.isUIActive()) {
        if (door.update(dt, tiles)) {
            saveMapTiles();
        }
        return;
    }

    updateEnemies(dt, player);
    updateDroppedItems(px, py, pw, ph);
    updatePotionCollision(px, py, pw, ph);

    activeDoorRow = -1;
    activeDoorCol = -1;
    activeTablePaperRow = -1;
    activeTablePaperCol = -1;

    int row, col;
    if (door.findActiveDoor(tiles, rows, cols, px, py, pw, ph, row, col)) {
        activeDoorRow = row;
        activeDoorCol = col;
    }

    if (tablePaper.findActiveTable(tiles, rows, cols, px, py, pw, ph, row, col)) {
        activeTablePaperRow = row;
        activeTablePaperCol = col;
    }

    if (activeGroundIdx >= 0) {
        activeTablePaperRow = -1;
        activeTablePaperCol = -1;
    }
}

void CavesOfChaos::handleEvent(const SDL_Event& event, Player& player) {
    bag.handleEvent(event);

    std::string droppedName;
    if (bag.consumeDrop(droppedName)) {
        if (droppedName == "Paper") {
            float dropX = player.getX() + 80.0f;
            float dropY = player.getY();
            spawnDroppedItem("Paper", 0.01f, dropX, dropY,
                             paperIconTex, paperIconW, paperIconH);
        }
        else if (droppedName == "Potion HP minor") {
            float dropX = player.getX() + 80.0f;
            float dropY = player.getY();
            spawnDroppedItem("Potion HP minor", 0.10f, dropX, dropY,
                             potionIconTex, potionIconW, potionIconH);
        }
    }

    std::string useName;
    if (bag.consumeUse(useName)) {
        if (useName == "Potion HP minor") {
            potionItem.use(player);
            bag.setItemStateByName("Potion HP minor", "vazio");
        }
    }

    if (bag.isOpen()) return;

    if (door.isUIActive()) {
        door.handleEvent(event);
        return;
    }

    if (event.type == SDL_KEYDOWN && event.key.repeat == 0) {
        if (event.key.keysym.scancode == SDL_SCANCODE_Q) {
            if (activeDoorRow >= 0 && activeDoorCol >= 0) {
                int id = door.charToId(tiles[activeDoorRow][activeDoorCol]);
                if (door.startRoll(activeDoorRow, activeDoorCol, id,
                                   tiles, player.strength)) {
                    saveMapTiles();
                }
                activeDoorRow = -1;
                activeDoorCol = -1;
            }
        }
        else if (event.key.keysym.scancode == SDL_SCANCODE_R) {
            if (activePotionRow >= 0 && activePotionCol >= 0) {
                bag.addItem("Potion HP minor", 0.10f, 1, "cheio");
                tiles[activePotionRow][activePotionCol] = '1';
                saveMapTiles();
                activePotionRow = -1;
                activePotionCol = -1;
            }
            else if (activeGroundIdx >= 0 &&
                     activeGroundIdx < static_cast<int>(droppedItems.size())) {
                bag.addItem(droppedItems[activeGroundIdx].name,
                            droppedItems[activeGroundIdx].weight, 1);
                droppedItems.erase(droppedItems.begin() + activeGroundIdx);
                activeGroundIdx = -1;
                saveDroppedItems();
            }
            else if (activeTablePaperRow >= 0 && activeTablePaperCol >= 0) {
                if (tablePaper.collect(tiles, activeTablePaperRow,
                                       activeTablePaperCol, bag)) {
                    saveMapTiles();
                }
                activeTablePaperRow = -1;
                activeTablePaperCol = -1;
            }
        }
    }
}

void CavesOfChaos::render(SDL_Renderer* renderer, int camX, int camY, Player* player) {
    SDL_Rect dst;
    dst.w = TILE_SIZE;
    dst.h = TILE_SIZE;

    int startCol = camX / TILE_SIZE;
    int startRow = camY / TILE_SIZE;
    int endCol   = (camX + 1400) / TILE_SIZE + 1;
    int endRow   = (camY + 850) / TILE_SIZE + 1;

    if (startCol < 0) startCol = 0;
    if (startRow < 0) startRow = 0;
    if (endCol > cols) endCol = cols;
    if (endRow > rows) endRow = rows;

    for (int y = startRow; y < endRow; ++y) {
        for (int x = startCol; x < endCol; ++x) {
            dst.x = x * TILE_SIZE - camX;
            dst.y = y * TILE_SIZE - camY;

            char c = tiles[y][x];
            SDL_Texture* tex = nullptr;

            if (c >= '1' && c <= '9') {
                tex = floor.getTexture(c - '0');
            } else if (c >= 'A' && c <= 'Z') {
                tex = wall.getTexture(c - 'A' + 1);
            } else if (tableEmpty.isTable(c)) {
                tex = tableEmpty.getTexture(tableEmpty.charToId(c));
            } else if (tablePaper.isTable(c)) {
                tex = tablePaper.getTexture(tablePaper.charToId(c));
            } else if (door.isDoor(c)) {
                tex = door.getTexture(door.charToId(c));
            } else if (chair.isChair(c)) {
                tex = chair.getTexture(chair.charToId(c));
            } else if (boockcase.isPart1(c)) {
                tex = boockcase.getPart1Texture();
            } else if (boockcase.isPart2(c)) {
                tex = boockcase.getPart2Texture();
            }
            if (tex) SDL_RenderCopy(renderer, tex, nullptr, &dst);
        }
    }

    renderDroppedItems(renderer, camX, camY);
    renderPotionTiles(renderer, camX, camY);

    struct Drawable {
        float y;
        CombatEnemy* enemy;
        Player* player;
    };

    std::vector<Drawable> drawables;
    drawables.reserve(ogros.size() + rats.size() + (player ? 1 : 0));

    for (auto& o : ogros) {
        if (o && o->isAlive()) {
            drawables.push_back({o->getY(), o.get(), nullptr});
        }
    }
    for (auto& rat : rats) {
        if (rat && rat->isAlive()) {
            drawables.push_back({rat->getY(), rat.get(), nullptr});
        }
    }
    if (player) {
        drawables.push_back({player->getY(), nullptr, player});
    }

    std::sort(drawables.begin(), drawables.end(),
              [](const Drawable& a, const Drawable& b) {
                  return a.y < b.y;
              });

    for (auto& d : drawables) {
        if (d.enemy) d.enemy->render(renderer, camX, camY);
        else if (d.player) d.player->render(renderer, camX, camY);
    }

    if (player) {
        player->renderNameplate(renderer, camX, camY);
    }

    if (activeDoorRow >= 0 && activeDoorCol >= 0
        && !door.isUIActive() && player != nullptr) {
        const int doorId =
            door.charToId(tiles[activeDoorRow][activeDoorCol]);
        door.renderPrompt(renderer, camX, camY, *player, doorId);
    }

    if (activeTablePaperRow >= 0 && activeTablePaperCol >= 0 && player != nullptr) {
        tablePaper.renderPrompt(renderer, camX, camY, *player);
    }

    if (activeGroundIdx >= 0 && player != nullptr) {
        renderGroundPrompt(renderer, camX, camY, *player);
    }

    if (activePotionRow >= 0 && activePotionCol >= 0 && player != nullptr) {
        renderPotionPrompt(renderer, camX, camY, *player);
    }
}

void CavesOfChaos::renderOverlay(SDL_Renderer* renderer) {
    door.renderUI(renderer);
    bag.renderUI(renderer);
}

bool CavesOfChaos::collides(float px, float py, int pw, int ph) const {
    int left   = static_cast<int>(px) / TILE_SIZE;
    int top    = static_cast<int>(py) / TILE_SIZE;
    int right  = static_cast<int>(px + pw - 1) / TILE_SIZE;
    int bottom = static_cast<int>(py + ph - 1) / TILE_SIZE;

    for (int ty = top; ty <= bottom; ++ty) {
        for (int tx = left; tx <= right; ++tx) {
            if (tx < 0 || ty < 0 || tx >= cols || ty >= rows)
                return true;

            char c = tiles[ty][tx];
            if (boockcase.isPart1(c)) continue;
            if (boockcase.isPart2(c)) return true;
            if (c >= 'A' && c <= 'Z') return true;
            if (tableEmpty.isTable(c)) return true;
            if (tablePaper.isTable(c)) return true;
            if (chair.isChair(c)) return true;
            if (door.isDoor(c))        return true;
            if (c == 'p')              continue;
            if (c < '1' || c > '9')    return true;
        }
    }
    return false;
}

bool CavesOfChaos::collidesWithEnemies(float px, float py, int pw, int ph) const {
    for (const auto& o : ogros) {
        if (o && o->isAlive()) {
            float hx, hy; int hw, hh;
            o->getHitbox(hx, hy, hw, hh);
            if (px < hx + hw && px + pw > hx &&
                py < hy + hh && py + ph > hy) {
                return true;
            }
        }
    }
    for (const auto& rat : rats) {
        if (rat && rat->isAlive()) {
            float hx, hy;
            int hw, hh;
            rat->getHitbox(hx, hy, hw, hh);
            if (px < hx + hw && px + pw > hx &&
                py < hy + hh && py + ph > hy) {
                return true;
            }
        }
    }
    return false;
}

int CavesOfChaos::countEnemiesTouching(float px, float py, int pw, int ph) const {
    return static_cast<int>(enemiesTouching(px, py, pw, ph).size());
}

std::vector<CombatEnemy*> CavesOfChaos::enemiesTouching(
    float px, float py, int pw, int ph) const {
    constexpr float contactMargin = 8.0f;
    std::vector<CombatEnemy*> touchingEnemies;
    for (const auto& ogro : ogros) {
        if (!ogro || !ogro->isAlive()) continue;

        float enemyX, enemyY;
        int enemyW, enemyH;
        ogro->getHitbox(enemyX, enemyY, enemyW, enemyH);
        const bool touching =
            px - contactMargin < enemyX + enemyW &&
            px + pw + contactMargin > enemyX &&
            py - contactMargin < enemyY + enemyH &&
            py + ph + contactMargin > enemyY;
        if (touching) touchingEnemies.push_back(ogro.get());
    }
    for (const auto& rat : rats) {
        if (!rat || !rat->isAlive()) continue;

        float enemyX, enemyY;
        int enemyW, enemyH;
        rat->getHitbox(enemyX, enemyY, enemyW, enemyH);
        const bool touching =
            px - contactMargin < enemyX + enemyW &&
            px + pw + contactMargin > enemyX &&
            py - contactMargin < enemyY + enemyH &&
            py + ph + contactMargin > enemyY;
        if (touching) touchingEnemies.push_back(rat.get());
    }
    return touchingEnemies;
}

void CavesOfChaos::removeDefeatedEnemies() {
    ogros.erase(
        std::remove_if(ogros.begin(), ogros.end(),
            [](const std::unique_ptr<Ogro>& ogro) {
                return !ogro || !ogro->isAlive();
            }),
        ogros.end());
    rats.erase(
        std::remove_if(rats.begin(), rats.end(),
            [](const std::unique_ptr<Rat>& rat) {
                return !rat || !rat->isAlive();
            }),
        rats.end());
}

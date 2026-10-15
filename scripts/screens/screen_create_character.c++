#include "scripts/screens/screen_create_character.h"
#include "scripts/resources/resource_archive.h"
#include "scripts/resources/player_save.h"
#include "scripts/dice/dice_1d6.h"
#include "scripts/skills/paladino_devotion.h"
#include "scripts/skills/barbarian_berserker.h"
#include "scripts/wearpons/iron_hammer.h"
#include "scripts/wearpons/iron_shield.h"
#include "scripts/armors/plate_armor.h"
#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <ctime>
#include <cstring>
#include <iostream>
#include <map>
#include <string>
#include <vector>

static bool createCharRngSeeded = false;

namespace {

const char* BTN_CLASS_WIZARD    = "resources/interfaces/buttons/button_wizard_idle.png";
const char* BTN_CLASS_LADINO    = "resources/interfaces/buttons/button_ladino_idle.png";
const char* BTN_CLASS_SORCERER  = "resources/interfaces/buttons/button_sourcerer_idle.png";
const char* BTN_CLASS_BARBARIAN = "resources/interfaces/buttons/button_barbarian_idle.png";
const char* BTN_CLASS_PALADIN   = "resources/interfaces/buttons/button_paladin_idle.png";

const char* BTN_SUB_EVOKER            = "resources/interfaces/buttons/button_evoker_idle.png";
const char* BTN_SUB_ABJURER           = "resources/interfaces/buttons/button_abjurer_idle.png";
const char* BTN_SUB_ASSASSIN          = "resources/interfaces/buttons/button_assassin_idle.png";
const char* BTN_SUB_ARCANE_TRICKSTER  = "resources/interfaces/buttons/button_arcane_trickster_idle.png";
const char* BTN_SUB_ABERRANT_SOURCERY = "resources/interfaces/buttons/button_aberrant_sourcery_idle.png";
const char* BTN_SUB_PATH_BERSERKER    = "resources/interfaces/buttons/button_path_berserker_idle.png";
const char* BTN_SUB_PATH_WILDHEART    = "resources/interfaces/buttons/button_path_wild_idle.png";
const char* BTN_SUB_OATH_GLORY        = "resources/interfaces/buttons/button_oath_glory_idle.png";
const char* BTN_SUB_OATH_DEVOTION     = "resources/interfaces/buttons/button_oath_devotion_idle.png";

const char* BTN_RACE_HUMAN   = "resources/interfaces/buttons/button_human_idle.png";
const char* BTN_RACE_ELF     = "resources/interfaces/buttons/button_elf_idle.png";
const char* BTN_RACE_GOBLIN  = "resources/interfaces/buttons/button_goblin_idle.png";
const char* BTN_RACE_DEMOGON = "resources/interfaces/buttons/button_demogon_idle.png";

const char* BTN_SUBRACE_ALTELF = "resources/interfaces/buttons/button_altelf_idle.png";
const char* BTN_SUBRACE_DROW   = "resources/interfaces/buttons/button_drow_idle.png";

const char* BTN_GENDER_MALE   = "resources/interfaces/buttons/button_genero_male.png";
const char* BTN_GENDER_FEMALE = "resources/interfaces/buttons/button_genero_female.png";
const char* BTN_ROLL_DICE     = "resources/interfaces/buttons/button_roll_dice.png";
const char* BTN_CONFIRM_IDLE  = "resources/interfaces/buttons/button_confirm_idle.png";
const char* BTN_RETURN_IDLE   = "resources/interfaces/buttons/button_return_2_idle.png";

const std::map<std::string, std::string> PT_TO_ID = {
    {"mago",             "wizard"},
    {"ladino",           "ladino"},
    {"feiticeiro",       "sorcerer"},
    {"barbaro",          "barbarian"},
    {"paladino",         "paladin"},
    {"paladin",          "paladin"},
    {"evoker",           "evoker"},
    {"abjurer",          "abjurer"},
    {"assassin",         "assassin"},
    {"aberrant_soucery", "aberrant_soucery"},
    {"aberrant",         "aberrant_soucery"},
    {"berserker",        "path_berserker"},
    {"wildheart",        "path_wildheart"},
    {"arcane_trickster", "arcane_trickster"},
    {"path_berserker",   "path_berserker"},
    {"path_wildheart",   "path_wildheart"},
    {"oath_glory",       "oath_glory"},
    {"oath_devotion",    "oath_devotion"},
    {"humano",           "human"},
    {"human",            "human"},
    {"elf",              "elf"},
    {"goblin",           "goblin"},
    {"demogon",          "demogon"},
    {"altelf",           "altelf"},
    {"drow",             "drow"},
    {"homem",            "male"},
    {"mulher",           "female"},
};

const std::map<std::string, std::string> BUTTON_IMAGE = {
    {"wizard",           BTN_CLASS_WIZARD},
    {"ladino",           BTN_CLASS_LADINO},
    {"sorcerer",         BTN_CLASS_SORCERER},
    {"barbarian",        BTN_CLASS_BARBARIAN},
    {"paladin",          BTN_CLASS_PALADIN},
    {"evoker",           BTN_SUB_EVOKER},
    {"abjurer",          BTN_SUB_ABJURER},
    {"assassin",         BTN_SUB_ASSASSIN},
    {"arcane_trickster", BTN_SUB_ARCANE_TRICKSTER},
    {"aberrant_soucery", BTN_SUB_ABERRANT_SOURCERY},
    {"path_berserker",   BTN_SUB_PATH_BERSERKER},
    {"path_wildheart",   BTN_SUB_PATH_WILDHEART},
    {"oath_glory",       BTN_SUB_OATH_GLORY},
    {"oath_devotion",    BTN_SUB_OATH_DEVOTION},
    {"human",            BTN_RACE_HUMAN},
    {"elf",              BTN_RACE_ELF},
    {"goblin",           BTN_RACE_GOBLIN},
    {"demogon",          BTN_RACE_DEMOGON},
    {"altelf",           BTN_SUBRACE_ALTELF},
    {"drow",             BTN_SUBRACE_DROW},
    {"male",             BTN_GENDER_MALE},
    {"female",           BTN_GENDER_FEMALE},
    {"roll_dice",        BTN_ROLL_DICE},
    {"confirm",          BTN_CONFIRM_IDLE},
    {"return",           BTN_RETURN_IDLE},
};

const std::vector<std::string> CLASSES = {
    "wizard", "ladino", "sorcerer", "barbarian", "paladin"
};

const std::vector<std::string> SUBCLASSES = {
    "evoker", "abjurer",
    "assassin", "arcane_trickster",
    "aberrant_soucery",
    "path_berserker", "path_wildheart",
    "oath_glory", "oath_devotion"
};

const std::vector<std::string> RACES = {
    "human", "elf", "goblin", "demogon"
};

const std::vector<std::string> SUBRACES = {
    "altelf", "drow"
};

const std::vector<std::string> GENDERS = {
    "male", "female"
};

const std::map<std::string, std::vector<std::string> > CLASS_SUBS = {
    {"wizard",    {"evoker", "abjurer"}},
    {"ladino",    {"assassin", "arcane_trickster"}},
    {"sorcerer",  {"aberrant_soucery"}},
    {"barbarian", {"path_berserker", "path_wildheart"}},
    {"paladin",   {"oath_glory", "oath_devotion"}},
};

std::string firstSubclassForClass(const std::string& cls) {
    const auto it = CLASS_SUBS.find(cls);
    return it == CLASS_SUBS.end() || it->second.empty()
        ? std::string() : it->second.front();
}

const std::map<std::string, std::vector<std::string> > RACE_SUBS = {
    {"elf", {"altelf", "drow"}},
};

const std::map<std::string, std::vector<std::string> > RACE_CLASSES = {
    {"human",   {"wizard", "ladino", "sorcerer", "barbarian", "paladin"}},
    {"elf",     {"ladino"}},
    {"altelf",  {"wizard", "ladino"}},
    {"drow",    {"ladino"}},
    {"goblin",  {"ladino", "sorcerer"}},
    {"demogon", {"ladino", "sorcerer"}},
};

bool raceSupportsClass(const std::string& race, const std::string& cls) {
    std::map<std::string, std::vector<std::string> >::const_iterator it =
        RACE_CLASSES.find(race);
    if (it != RACE_CLASSES.end() &&
        std::find(it->second.begin(), it->second.end(), cls) !=
            it->second.end()) {
        return true;
    }
    if (race == "elf") {
        const std::string subraces[] = {"altelf", "drow"};
        for (const std::string& subrace : subraces) {
            it = RACE_CLASSES.find(subrace);
            if (it != RACE_CLASSES.end() &&
                std::find(it->second.begin(), it->second.end(), cls) !=
                    it->second.end()) {
                return true;
            }
        }
    }
    return false;
}

const std::map<std::string, std::string> BACKGROUNDS = {
    {"elf|altelf|wizard|abjurer|female", "resources/interfaces/scenes/creation/creation_altelf_mago_abjurer_mulher.png"},
    {"elf|altelf|wizard|evoker|female", "resources/interfaces/scenes/creation/creation_altelf_mago_evoker_mulher.png"},

    {"demogon||ladino||male", "resources/interfaces/scenes/creation/creation_demogon_ladino_homem.png"},
    {"demogon||ladino||female", "resources/interfaces/scenes/creation/creation_demogon_ladino_mulher.png"},

    {"elf|drow|ladino|arcane_trickster|female", "resources/interfaces/scenes/creation/creation_drow_ladino_arcane_trickster_mulher.png"},
    {"elf|drow|ladino|assassin|female", "resources/interfaces/scenes/creation/creation_drow_ladino_assassin_mulher.png"},
    {"elf|drow|ladino||female", "resources/interfaces/scenes/creation/creation_drow_ladino_mulher.png"},

    {"goblin||sorcerer||male", "resources/interfaces/scenes/creation/creation_goblin_feiticeiro_homem.png"},
    {"goblin||sorcerer||female", "resources/interfaces/scenes/creation/creation_goblin_feiticeiro_mulher.png"},
    {"goblin||ladino||male", "resources/interfaces/scenes/creation/creation_goblin_ladino_homem.png"},
    {"goblin||ladino||female", "resources/interfaces/scenes/creation/creation_goblin_ladino_mulher.png"},

    {"human||barbarian|path_berserker|male", "resources/interfaces/scenes/creation/creation_humano_barbaro_homem.png"},
    {"human||barbarian|path_berserker|female", "resources/interfaces/scenes/creation/creation_humano_barbaro_mulher.png"},
    {"human||barbarian|path_wildheart|male", "resources/interfaces/scenes/creation/creation_humano_barbaro_homem.png"},
    {"human||barbarian|path_wildheart|female", "resources/interfaces/scenes/creation/creation_humano_barbaro_mulher.png"},

    {"human||sorcerer|aberrant_soucery|male", "resources/interfaces/scenes/creation/creation_humano_feiticeiro_aberrant_homem.png"},
    {"human||sorcerer|aberrant_soucery|female", "resources/interfaces/scenes/creation/creation_humano_feiticeiro_aberrant_mulher.png"},
    {"human||sorcerer||male", "resources/interfaces/scenes/creation/creation_humano_feiticeiro_homem.png"},
    {"human||sorcerer||female", "resources/interfaces/scenes/creation/creation_humano_feiticeiro_mulher.png"},

    {"human||ladino||male", "resources/interfaces/scenes/creation/creation_humano_ladino_homem.png"},
    {"human||ladino||female", "resources/interfaces/scenes/creation/creation_humano_ladino_mulher.png"},
    {"human||wizard||male", "resources/interfaces/scenes/creation/creation_humano_mago_homem.png"},
    {"human||wizard||female", "resources/interfaces/scenes/creation/creation_humano_mago_mulher.png"},
    {"human||paladin||male", "resources/interfaces/scenes/creation/creation_humano_paladino_homem.png"},
    {"human||paladin||female", "resources/interfaces/scenes/creation/creation_humano_paladino_mulher.png"},
};

}

TelaCreateCharacter::TelaCreateCharacter(SDL_Renderer* renderer,
                                         int screenW, int screenH)
    : renderer(renderer),
      screenW(screenW),
      screenH(screenH),
      running(true),
      startGame(false),
      returnToIntro(false),
      backgroundTex(nullptr)
      , hasSelectedButton(false),
      selectedButtonKind(Kind::CLASS),
      nameFont(nullptr),
      attributeFont(nullptr),
      nameInputFocused(false),
      rolledAttributes{{0, 0, 0, 0, 0, 0}},
      attributesRolled(false)
{
    nameInputRect = { screenW - 320, 24, 290, 48 };
    if (!createCharRngSeeded) {
        std::srand(static_cast<unsigned>(std::time(nullptr)));
        createCharRngSeeded = true;
    }

    backgroundPool.push_back("resources/interfaces/scenes/creation/creation_altelf_mago_abjurer_mulher.png");
    backgroundPool.push_back("resources/interfaces/scenes/creation/creation_altelf_mago_evoker_mulher.png");
    backgroundPool.push_back("resources/interfaces/scenes/creation/creation_demogon_ladino_homem.png");
    backgroundPool.push_back("resources/interfaces/scenes/creation/creation_demogon_ladino_mulher.png");
    backgroundPool.push_back("resources/interfaces/scenes/creation/creation_drow_ladino_arcane_trickster_mulher.png");
    backgroundPool.push_back("resources/interfaces/scenes/creation/creation_drow_ladino_assassin_mulher.png");
    backgroundPool.push_back("resources/interfaces/scenes/creation/creation_drow_ladino_mulher.png");
    backgroundPool.push_back("resources/interfaces/scenes/creation/creation_goblin_feiticeiro_homem.png");
    backgroundPool.push_back("resources/interfaces/scenes/creation/creation_goblin_feiticeiro_mulher.png");
    backgroundPool.push_back("resources/interfaces/scenes/creation/creation_goblin_ladino_homem.png");
    backgroundPool.push_back("resources/interfaces/scenes/creation/creation_goblin_ladino_mulher.png");
    backgroundPool.push_back("resources/interfaces/scenes/creation/creation_humano_barbaro_berserker_homem.png");
    backgroundPool.push_back("resources/interfaces/scenes/creation/creation_humano_barbaro_berserker_mulher.png");
    backgroundPool.push_back("resources/interfaces/scenes/creation/creation_humano_barbaro_wildheart_homem.png");
    backgroundPool.push_back("resources/interfaces/scenes/creation/creation_humano_barbaro_wildheart_mulher.png");
    backgroundPool.push_back("resources/interfaces/scenes/creation/creation_humano_feiticeiro_aberrant_homem.png");
    backgroundPool.push_back("resources/interfaces/scenes/creation/creation_humano_feiticeiro_aberrant_mulher.png");
    backgroundPool.push_back("resources/interfaces/scenes/creation/creation_humano_feiticeiro_homem.png");
    backgroundPool.push_back("resources/interfaces/scenes/creation/creation_humano_feiticeiro_mulher.png");
    backgroundPool.push_back("resources/interfaces/scenes/creation/creation_humano_ladino_homem.png");
    backgroundPool.push_back("resources/interfaces/scenes/creation/creation_humano_ladino_mulher.png");
    backgroundPool.push_back("resources/interfaces/scenes/creation/creation_humano_mago_homem.png");
    backgroundPool.push_back("resources/interfaces/scenes/creation/creation_humano_mago_mulher.png");
    backgroundPool.push_back("resources/interfaces/scenes/creation/creation_humano_paladino_homem.png");
    backgroundPool.push_back("resources/interfaces/scenes/creation/creation_humano_paladino_mulher.png");
}

TelaCreateCharacter::~TelaCreateCharacter() {
    clearButtons();
    if (backgroundTex) SDL_DestroyTexture(backgroundTex);
    if (nameFont) TTF_CloseFont(nameFont);
    if (attributeFont) TTF_CloseFont(attributeFont);
    SDL_StopTextInput();
}

void TelaCreateCharacter::clearButtons() {
    for (size_t i = 0; i < buttons.size(); ++i) {
        if (buttons[i].tex)  SDL_DestroyTexture(buttons[i].tex);
        if (buttons[i].hoverTex) SDL_DestroyTexture(buttons[i].hoverTex);
        if (buttons[i].mask) SDL_FreeSurface(buttons[i].mask);
    }
    buttons.clear();
}

void TelaCreateCharacter::parseRandomBackground(const std::string& path,
                                                Selection& out) const
{
    out = Selection();

    std::string base = path;
    size_t slash = base.find_last_of('/');
    if (slash != std::string::npos) base = base.substr(slash + 1);

    const std::string prefix = "creation_";
    const std::string suffix = ".png";

    if (base.size() < prefix.size() + suffix.size()) return;
    if (base.substr(0, prefix.size()) != prefix) return;
    if (base.substr(base.size() - suffix.size()) != suffix) return;

    std::string mid = base.substr(prefix.size(),
                                  base.size() - prefix.size() - suffix.size());

    std::vector<std::string> parts;
    size_t start = 0;
    while (true) {
        size_t pos = mid.find('_', start);
        if (pos == std::string::npos) {
            parts.push_back(mid.substr(start));
            break;
        }
        parts.push_back(mid.substr(start, pos - start));
        start = pos + 1;
    }

    if (parts.size() < 3) return;

    const std::map<std::string, std::string>::const_iterator raceIt =
        PT_TO_ID.find(parts.front());
    if (raceIt == PT_TO_ID.end()) return;

    if (raceIt->second == "altelf" || raceIt->second == "drow") {
        out.race = "elf";
        out.subrace = raceIt->second;
    } else {
        out.race = raceIt->second;
    }

    const std::map<std::string, std::string>::const_iterator genderIt =
        PT_TO_ID.find(parts.back());
    if (genderIt == PT_TO_ID.end()
        || std::find(GENDERS.begin(), GENDERS.end(), genderIt->second) == GENDERS.end()) {
        return;
    }
    out.gender = genderIt->second;

    const std::map<std::string, std::string>::const_iterator classIt =
        PT_TO_ID.find(parts[1]);
    if (classIt == PT_TO_ID.end()) return;
    if (std::find(CLASSES.begin(), CLASSES.end(), classIt->second) == CLASSES.end()) {
        return;
    }
    out.cls = classIt->second;

    if (parts.size() > 3) {
        std::string subclassName;
        for (size_t i = 2; i + 1 < parts.size(); ++i) {
            if (!subclassName.empty()) subclassName += "_";
            subclassName += parts[i];
        }

        const std::map<std::string, std::string>::const_iterator subclassIt =
            PT_TO_ID.find(subclassName);
        if (subclassIt != PT_TO_ID.end()
            && std::find(SUBCLASSES.begin(), SUBCLASSES.end(), subclassIt->second) != SUBCLASSES.end()) {
            out.subclass = subclassIt->second;
        }
    }
}

bool TelaCreateCharacter::loadBackgroundTexture(const std::string& path) {
    SDL_Surface* surf = resource_archive::loadResourceImage(path);
    if (!surf) {
        std::cerr << "Falha ao carregar background " << path
                  << ": " << IMG_GetError() << std::endl;
        return false;
    }

    SDL_Surface* scaled = SDL_CreateRGBSurfaceWithFormat(
        0, screenW, screenH, 32, SDL_PIXELFORMAT_RGBA32);
    if (!scaled) {
        SDL_FreeSurface(surf);
        return false;
    }

    SDL_SetSurfaceBlendMode(surf, SDL_BLENDMODE_BLEND);
    SDL_BlitScaled(surf, nullptr, scaled, nullptr);
    SDL_SetSurfaceBlendMode(scaled, SDL_BLENDMODE_BLEND);

    SDL_Texture* newTex = SDL_CreateTextureFromSurface(renderer, scaled);
    SDL_FreeSurface(scaled);
    SDL_FreeSurface(surf);

    if (!newTex) return false;

    SDL_SetTextureScaleMode(newTex, SDL_ScaleModeNearest);

    if (backgroundTex) SDL_DestroyTexture(backgroundTex);
    backgroundTex = newTex;
    currentBackgroundPath = path;
    return true;
}

void TelaCreateCharacter::loadRandomBackground() {
    if (backgroundPool.empty()) return;
    int idx = std::rand() % static_cast<int>(backgroundPool.size());
    const std::string& path = backgroundPool[idx];
    parseRandomBackground(path, selection);
    loadBackgroundTexture(path);
}

bool TelaCreateCharacter::loadButtonTexture(const std::string& path,
                                            SDL_Texture*& outTex,
                                            SDL_Surface*& outMask,
                                            int& outW, int& outH)
{
    outTex  = nullptr;
    outMask = nullptr;
    outW    = 0;
    outH    = 0;

    SDL_Surface* surf = resource_archive::loadResourceImage(path);
    if (!surf) return false;

    SDL_Surface* converted = SDL_ConvertSurfaceFormat(
        surf, SDL_PIXELFORMAT_RGBA32, 0);
    SDL_FreeSurface(surf);
    if (!converted) return false;

    SDL_Surface* scaled = SDL_CreateRGBSurfaceWithFormat(
        0, screenW, screenH, 32, SDL_PIXELFORMAT_RGBA32);
    if (!scaled) {
        SDL_FreeSurface(converted);
        return false;
    }

    SDL_SetSurfaceBlendMode(converted, SDL_BLENDMODE_BLEND);
    if (SDL_BlitScaled(converted, nullptr, scaled, nullptr) < 0) {
        SDL_FreeSurface(converted);
        SDL_FreeSurface(scaled);
        return false;
    }
    SDL_FreeSurface(converted);

    outTex = SDL_CreateTextureFromSurface(renderer, scaled);
    if (!outTex) {
        SDL_FreeSurface(scaled);
        return false;
    }

    SDL_SetTextureScaleMode(outTex, SDL_ScaleModeNearest);
    outW = screenW;
    outH = screenH;
    outMask = scaled;
    return true;
}

void TelaCreateCharacter::addButtonLayers(const std::vector<std::string>& ids,
                                          Kind kind)
{
    if (ids.empty()) return;

    for (size_t i = 0; i < ids.size(); ++i) {
        std::map<std::string, std::string>::const_iterator it =
            BUTTON_IMAGE.find(ids[i]);
        if (it == BUTTON_IMAGE.end()) continue;

        SDL_Texture* tex = nullptr;
        SDL_Surface* mask = nullptr;
        int width = 0;
        int height = 0;
        if (!loadButtonTexture(it->second, tex, mask, width, height)) {
            std::cerr << "Falha ao carregar botao " << it->second
                      << ": " << IMG_GetError() << std::endl;
            continue;
        }

        Button b;
        b.kind     = kind;
        b.id       = ids[i];
        b.idlePath = it->second;
        b.tex      = tex;
        b.mask     = mask;
        b.hoverTex = nullptr;
        b.hovered  = false;
        b.enabled  = true;
        b.selected = hasSelectedButton &&
                     selectedButtonKind == kind &&
                     selectedButtonId == ids[i];
        if (kind == Kind::GENDER && selection.gender == ids[i]) {
            b.selected = true;
        } else if (kind == Kind::SUBCLASS &&
                   selection.cls.size() > 0 &&
                   selection.subclass == ids[i]) {
            b.selected = true;
        }
        std::string hoverPath = it->second;
        const std::string idleSuffix = "_idle.png";
        const std::size_t suffixPos = hoverPath.rfind(idleSuffix);
        if (suffixPos != std::string::npos) {
            hoverPath.replace(suffixPos, idleSuffix.size(), "_hover.png");
            SDL_Surface* hoverMask = nullptr;
            int hoverW = 0;
            int hoverH = 0;
            if (!loadButtonTexture(hoverPath, b.hoverTex, hoverMask,
                                   hoverW, hoverH)) {
                b.hoverTex = nullptr;
            }
            if (hoverMask) SDL_FreeSurface(hoverMask);
        }
        b.rect.x = 0;
        b.rect.y = 0;
        b.rect.w = width;
        b.rect.h = height;
        b.enabled = isButtonEnabled(b);

        buttons.push_back(b);
    }
}

bool TelaCreateCharacter::isButtonEnabled(const Button& button) const {
    if (button.kind == Kind::CLASS && !selection.race.empty()) {
        const std::string selectedRace = selection.subrace.empty()
            ? selection.race : selection.subrace;
        return raceSupportsClass(selectedRace, button.id);
    }

    if (button.kind == Kind::RACE && !selection.cls.empty()) {
        return raceSupportsClass(button.id, selection.cls);
    }

    if (button.kind == Kind::SUBRACE &&
        selection.race == "elf" &&
        selection.cls == "wizard") {
        return button.id == "altelf";
    }

    return true;
}

void TelaCreateCharacter::rebuildButtons() {
    clearButtons();

    std::vector<std::string> availableSubs;
    std::map<std::string, std::vector<std::string> >::const_iterator itCS = CLASS_SUBS.find(selection.cls);
    if (itCS != CLASS_SUBS.end()) {
        availableSubs = itCS->second;
    }

    std::vector<std::string> availableSubraces;
    std::map<std::string, std::vector<std::string> >::const_iterator itRS = RACE_SUBS.find(selection.race);
    if (itRS != RACE_SUBS.end()) {
        availableSubraces = itRS->second;
    }

    addButtonLayers(CLASSES,           Kind::CLASS);
    addButtonLayers(availableSubs,     Kind::SUBCLASS);
    addButtonLayers(RACES,             Kind::RACE);
    addButtonLayers(availableSubraces, Kind::SUBRACE);
    addButtonLayers(GENDERS,           Kind::GENDER);
    addButtonLayers(std::vector<std::string>(1, "roll_dice"), Kind::ROLL);
    addButtonLayers(std::vector<std::string>(1, "confirm"), Kind::CONFIRM);
    addButtonLayers(std::vector<std::string>(1, "return"), Kind::RETURN);
}

bool TelaCreateCharacter::resolveBackground() {
    if (selection.race.empty() || selection.cls.empty()
        || selection.gender.empty()) {
        return false;
    }

    std::vector<Attempt> attempts;

    Attempt a1;
    a1.race     = selection.race;
    a1.subrace  = selection.subrace;
    a1.cls      = selection.cls;
    a1.subclass = selection.subclass;
    a1.gender   = selection.gender;
    attempts.push_back(a1);

    Attempt a2;
    a2.race     = selection.race;
    a2.subrace  = selection.subrace;
    a2.cls      = selection.cls;
    a2.subclass = "";
    a2.gender   = selection.gender;
    attempts.push_back(a2);

    Attempt a3;
    a3.race     = selection.race;
    a3.subrace  = "";
    a3.cls      = selection.cls;
    a3.subclass = selection.subclass;
    a3.gender   = selection.gender;
    attempts.push_back(a3);

    Attempt a4;
    a4.race     = selection.race;
    a4.subrace  = "";
    a4.cls      = selection.cls;
    a4.subclass = "";
    a4.gender   = selection.gender;
    attempts.push_back(a4);

    if (!selection.subrace.empty()) {
        Attempt a5;
        a5.race     = selection.subrace;
        a5.subrace  = "";
        a5.cls      = selection.cls;
        a5.subclass = selection.subclass;
        a5.gender   = selection.gender;
        attempts.push_back(a5);

        Attempt a6;
        a6.race     = selection.subrace;
        a6.subrace  = "";
        a6.cls      = selection.cls;
        a6.subclass = "";
        a6.gender   = selection.gender;
        attempts.push_back(a6);
    }

    for (size_t i = 0; i < attempts.size(); ++i) {
        std::string key;
        key += attempts[i].race;
        key += "|";
        key += attempts[i].subrace;
        key += "|";
        key += attempts[i].cls;
        key += "|";
        key += attempts[i].subclass;
        key += "|";
        key += attempts[i].gender;

        std::map<std::string, std::string>::const_iterator it = BACKGROUNDS.find(key);
        if (it != BACKGROUNDS.end()) {
            return loadBackgroundTexture(it->second);
        }
    }

    return false;
}

bool TelaCreateCharacter::updateBackgroundFromSelection() {
    if (resolveBackground()) return true;

    if (selection.subclass.empty()) {
        const std::map<std::string, std::vector<std::string> >::const_iterator
            subclasses = CLASS_SUBS.find(selection.cls);
        if (subclasses != CLASS_SUBS.end()) {
            for (const std::string& subclass : subclasses->second) {
                selection.subclass = subclass;
                if (resolveBackground()) return true;
            }
            selection.subclass.clear();
        }
    }

    std::cerr << "Nenhum background disponivel para a combinacao "
              << selection.race << "/" << selection.subrace << "/"
              << selection.cls << "/" << selection.subclass << "/"
              << selection.gender << std::endl;
    return false;
}

bool TelaCreateCharacter::isPixelOpaque(SDL_Surface* surf,
                                        int x, int y) const
{
    if (!surf) return false;
    if (x < 0 || y < 0 || x >= surf->w || y >= surf->h) return false;

    Uint32* pixels = static_cast<Uint32*>(surf->pixels);
    Uint32 pixel = pixels[y * surf->w + x];
    Uint8 a = static_cast<Uint8>(
        (pixel & surf->format->Amask) >> surf->format->Ashift);
    return a > 10;
}

bool TelaCreateCharacter::isPointOverButton(int mx, int my,
                                            const Button& btn) const
{
    if (mx < btn.rect.x || mx >= btn.rect.x + btn.rect.w) return false;
    if (my < btn.rect.y || my >= btn.rect.y + btn.rect.h) return false;

    if (!btn.mask) return true;

    int px = mx - btn.rect.x;
    int py = my - btn.rect.y;
    return isPixelOpaque(btn.mask, px, py);
}

void TelaCreateCharacter::handleButtonClick(const Button& btn) {
    if (!btn.enabled) return;

    if (btn.kind != Kind::GENDER) {
        hasSelectedButton = true;
        selectedButtonKind = btn.kind;
        selectedButtonId = btn.id;
    }

    if (btn.kind == Kind::RETURN) {
        returnToIntro = true;
        return;
    }
    if (btn.kind == Kind::CONFIRM) {
        createCharacterSave();
        return;
    }
    if (btn.kind == Kind::ROLL) {
        for (int& attribute : rolledAttributes) {
            attribute = Dice1d6::roll4d6DropLowest();
        }
        attributesRolled = true;
        statusMessage.clear();
        return;
    }

    if (currentBackgroundPath ==
            "resources/interfaces/scenes/creation/creation_altelf_mago_abjurer_mulher.png"
        && btn.kind == Kind::CLASS && btn.id == "ladino") {
        const Selection previous = selection;
        selection.race = "human";
        selection.subrace.clear();
        selection.cls = "ladino";
        selection.subclass = firstSubclassForClass(selection.cls);
        if (!updateBackgroundFromSelection()) selection = previous;
        rebuildButtons();
        return;
    }

    const Selection previous = selection;
    switch (btn.kind) {
        case Kind::CLASS:
            {
                const std::string selectedRace = selection.subrace.empty()
                    ? selection.race : selection.subrace;
                const std::map<std::string,
                    std::vector<std::string> >::const_iterator raceClasses =
                        RACE_CLASSES.find(selectedRace);
                if (raceClasses == RACE_CLASSES.end() ||
                    std::find(raceClasses->second.begin(),
                              raceClasses->second.end(), btn.id) ==
                        raceClasses->second.end()) {
                    return;
                }
            }
            selection.cls = btn.id;
            selection.subclass = firstSubclassForClass(selection.cls);
            break;

        case Kind::SUBCLASS:
            selection.subclass = btn.id;
            break;

        case Kind::RACE:
            selection.race = btn.id;
            selection.subrace.clear();
            if (selection.race == "elf") {
                if (selection.cls == "wizard") {
                    selection.subrace = "altelf";
                } else if (selection.cls == "ladino") {
                    selection.subrace = "drow";
                } else {
                    selection.subrace = "altelf";
                }
            }
            {
                const std::string selectedRace = selection.subrace.empty()
                    ? selection.race : selection.subrace;
                std::map<std::string,
                    std::vector<std::string> >::const_iterator itRC =
                        RACE_CLASSES.find(selectedRace);
                bool valid = false;
                if (itRC != RACE_CLASSES.end()) {
                    valid = std::find(itRC->second.begin(),
                                      itRC->second.end(),
                                      selection.cls) != itRC->second.end();
                }
                if (!valid) {
                    selection.cls.clear();
                    selection.subclass.clear();
                } else if (selection.subrace != previous.subrace ||
                           selection.race != previous.race) {
                    selection.subclass =
                        firstSubclassForClass(selection.cls);
                }
            }
            break;

        case Kind::SUBRACE:
            selection.subrace = btn.id;
            {
                std::map<std::string, std::vector<std::string> >::const_iterator itRC = RACE_CLASSES.find(selection.subrace);
                bool valid = false;
                if (itRC != RACE_CLASSES.end()) {
                    valid = std::find(itRC->second.begin(),
                                      itRC->second.end(),
                                      selection.cls) != itRC->second.end();
                }
                if (!valid) {
                    selection.cls.clear();
                    selection.subclass.clear();
                } else if (selection.subrace != previous.subrace) {
                    selection.subclass =
                        firstSubclassForClass(selection.cls);
                }
            }
            break;

        case Kind::GENDER:
            selection.gender = btn.id;
            break;

        case Kind::ROLL:
        case Kind::CONFIRM:
        case Kind::RETURN:
            break;
    }

    if (!updateBackgroundFromSelection()) selection = previous;
    rebuildButtons();
}

bool TelaCreateCharacter::loadNameFont() {
    if (TTF_WasInit() == 0 && TTF_Init() < 0) {
        std::cerr << "TTF_Init falhou: " << TTF_GetError() << std::endl;
        return false;
    }
    nameFont = resource_archive::openResourceFont(
        "resources/fonts/impact.ttf", 24);
    if (!nameFont) {
        std::cerr << "Falha ao carregar fonte do nome: "
                  << TTF_GetError() << std::endl;
        return false;
    }
    attributeFont = resource_archive::openResourceFont(
        "resources/fonts/impact.ttf", 18);
    if (!attributeFont) {
        std::cerr << "Falha ao carregar fonte dos atributos: "
                  << TTF_GetError() << std::endl;
        return false;
    }
    return true;
}

bool TelaCreateCharacter::createCharacterSave() {
    characterName.erase(
        characterName.begin(),
        std::find_if(characterName.begin(), characterName.end(),
            [](unsigned char c) { return !std::isspace(c); }));
    characterName.erase(
        std::find_if(characterName.rbegin(), characterName.rend(),
            [](unsigned char c) { return !std::isspace(c); }).base(),
        characterName.end());
    if (characterName.empty()) {
        statusMessage = "Digite o nome do personagem.";
        return false;
    }
    if (selection.race.empty() || selection.cls.empty() ||
        selection.gender.empty()) {
        statusMessage = "Selecione raca, classe e genero.";
        return false;
    }
    const auto classSubclasses = CLASS_SUBS.find(selection.cls);
    if (classSubclasses != CLASS_SUBS.end() &&
        std::find(classSubclasses->second.begin(),
                  classSubclasses->second.end(), selection.subclass) ==
            classSubclasses->second.end()) {
        statusMessage = "Selecione uma sub-classe.";
        return false;
    }

    player_save::Character character;
    character.name = characterName;
    character.cls = selection.cls;
    character.subclass = selection.subclass;
    character.race = selection.race;
    character.subrace = selection.subrace;
    character.gender = selection.gender;
    if (selection.cls == "paladin" || selection.cls == "paladino") {
        character.bag.push_back(
            {"Potion HP minor", 1, 0.10f, "cheio"});
        character.bag.push_back(
            {IronHammer::name(), 1, IronHammer::weight(), "equipado"});
        character.bag.push_back(
            {IronShield::name(), 1, IronShield::weight(), "equipado"});
        character.bag.push_back(
            {PlateArmor::name(), 1, PlateArmor::weight(), "equipado"});
    }
    paladino_devotion::populateSkills(
        selection.cls, selection.subclass, character.skills);
    barbarian_berserker::populateSkills(
        selection.cls, selection.subclass, character.skills);
    character.spritePath = player_save::spriteDirectory(
        selection.cls, selection.race, selection.subrace, selection.gender);
    if (character.spritePath.empty()) {
        statusMessage = "Nao foi possivel determinar os sprites do personagem.";
        return false;
    }
    character.level = 1;
    if (!attributesRolled) {
        statusMessage = "Role os atributos antes de confirmar.";
        return false;
    }
    character.attributes.strength = rolledAttributes[0];
    character.attributes.dexterity = rolledAttributes[1];
    character.attributes.constitution = rolledAttributes[2];
    character.attributes.intellect = rolledAttributes[3];
    character.attributes.wisdom = rolledAttributes[4];
    character.attributes.charisma = rolledAttributes[5];

    std::filesystem::path savePath;
    std::string error;
    if (!player_save::create(character, savePath, error)) {
        statusMessage = "Falha ao salvar personagem.";
        std::cerr << error << std::endl;
        return false;
    }

    statusMessage.clear();
    startGame = true;
    return true;
}

void TelaCreateCharacter::updateHover() {
    int mouseX = 0;
    int mouseY = 0;
    SDL_GetMouseState(&mouseX, &mouseY);
    for (size_t i = 0; i < buttons.size(); ++i) {
        buttons[i].hovered = buttons[i].enabled &&
            (buttons[i].selected ||
             (buttons[i].hoverTex &&
              isPointOverButton(mouseX, mouseY, buttons[i])));
    }
}

bool TelaCreateCharacter::init() {
    if (!loadNameFont()) return false;
    loadRandomBackground();
    rebuildButtons();
    return true;
}

void TelaCreateCharacter::handleEvent(const SDL_Event& event) {
    if (event.type == SDL_QUIT) {
        running = false;
        return;
    }

    if (event.type == SDL_KEYDOWN) {
        if (event.key.keysym.scancode == SDL_SCANCODE_ESCAPE) {
            returnToIntro = true;
        } else if (nameInputFocused &&
                   event.key.keysym.scancode == SDL_SCANCODE_BACKSPACE &&
                   !characterName.empty()) {
            characterName.pop_back();
            while (!characterName.empty() &&
                   (static_cast<unsigned char>(characterName.back()) & 0xC0) == 0x80) {
                characterName.pop_back();
            }
        } else if (nameInputFocused &&
                   event.key.keysym.scancode == SDL_SCANCODE_RETURN) {
            nameInputFocused = false;
            SDL_StopTextInput();
        }
        return;
    }

    if (event.type == SDL_TEXTINPUT && nameInputFocused) {
        const std::size_t maxNameLength = 48;
        if (characterName.size() + std::strlen(event.text.text) <= maxNameLength) {
            characterName += event.text.text;
        }
        return;
    }

    if (event.type == SDL_MOUSEBUTTONDOWN &&
        event.button.button == SDL_BUTTON_LEFT) {
        int mx = event.button.x;
        int my = event.button.y;
        if (mx >= nameInputRect.x && mx < nameInputRect.x + nameInputRect.w &&
            my >= nameInputRect.y && my < nameInputRect.y + nameInputRect.h) {
            nameInputFocused = true;
            SDL_StartTextInput();
            return;
        }
        nameInputFocused = false;
        SDL_StopTextInput();

        for (size_t i = buttons.size(); i > 0; --i) {
            if (buttons[i - 1].enabled &&
                isPointOverButton(mx, my, buttons[i - 1])) {
                handleButtonClick(buttons[i - 1]);
                return;
            }
        }
    }
}

void TelaCreateCharacter::update(float dt) {
    (void)dt;
    updateHover();
}

void TelaCreateCharacter::render() {
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);

    if (backgroundTex) {
        SDL_Rect dst = { 0, 0, screenW, screenH };
        SDL_RenderCopy(renderer, backgroundTex, nullptr, &dst);
    }

    for (size_t i = 0; i < buttons.size(); ++i) {
        const bool useHoverTexture = buttons[i].enabled &&
            (buttons[i].hovered || buttons[i].selected);
        SDL_Texture* texture = useHoverTexture && buttons[i].hoverTex
            ? buttons[i].hoverTex : buttons[i].tex;
        if (texture) {
            if (!buttons[i].enabled) {
                SDL_SetTextureColorMod(texture, 105, 105, 105);
            } else if (useHoverTexture &&
                       buttons[i].kind == Kind::GENDER &&
                       !buttons[i].hoverTex) {
                SDL_SetTextureColorMod(texture, 255, 220, 100);
            } else {
                SDL_SetTextureColorMod(texture, 255, 255, 255);
            }
            SDL_RenderCopy(renderer, texture, nullptr, &buttons[i].rect);
            SDL_SetTextureColorMod(texture, 255, 255, 255);
        }
    }

    SDL_SetRenderDrawColor(renderer, 20, 20, 20, 230);
    SDL_RenderFillRect(renderer, &nameInputRect);
    SDL_SetRenderDrawColor(renderer, nameInputFocused ? 255 : 180,
                           nameInputFocused ? 190 : 130, 40, 255);
    SDL_RenderDrawRect(renderer, &nameInputRect);

    const std::string displayedName = characterName.empty() && !nameInputFocused
        ? "Nome do personagem" : characterName;
    SDL_Color nameColor = {255, 220, 90, 255};
    SDL_Surface* nameSurface = TTF_RenderUTF8_Blended(
        nameFont, displayedName.c_str(), nameColor);
    if (nameSurface) {
        SDL_Texture* nameTexture = SDL_CreateTextureFromSurface(
            renderer, nameSurface);
        if (nameTexture) {
            SDL_Rect destination = {
                nameInputRect.x + 10, nameInputRect.y + 8,
                std::min(nameSurface->w, nameInputRect.w - 20),
                std::min(nameSurface->h, nameInputRect.h - 16)
            };
            SDL_RenderCopy(renderer, nameTexture, nullptr, &destination);
            SDL_DestroyTexture(nameTexture);
        }
        SDL_FreeSurface(nameSurface);
    }

    const int attributeCellWidth = 67;
    const int attributeRowWidth =
        attributeCellWidth * static_cast<int>(rolledAttributes.size());
    const int attributeRowX = screenW - 100 - attributeRowWidth;
    const int attributeRowY = nameInputRect.y + nameInputRect.h + 90;
    const SDL_Color attributeValueColor = {255, 255, 255, 255};
    for (std::size_t i = 0; i < rolledAttributes.size(); ++i) {
        const std::string value = attributesRolled
            ? std::to_string(rolledAttributes[i]) : "--";
        SDL_Surface* valueSurface = TTF_RenderUTF8_Blended(
            attributeFont, value.c_str(), attributeValueColor);
        if (valueSurface) {
            SDL_Texture* valueTexture =
                SDL_CreateTextureFromSurface(renderer, valueSurface);
            if (valueTexture) {
                SDL_Rect destination = {
                    attributeRowX +
                        static_cast<int>(i) * attributeCellWidth +
                        (attributeCellWidth - valueSurface->w) / 2,
                    attributeRowY,
                    valueSurface->w, valueSurface->h
                };
                SDL_RenderCopy(renderer, valueTexture, nullptr, &destination);
                SDL_DestroyTexture(valueTexture);
            }
            SDL_FreeSurface(valueSurface);
        }
    }

    if (!statusMessage.empty() && nameFont) {
        SDL_Color errorColor = {255, 100, 100, 255};
        SDL_Surface* statusSurface = TTF_RenderUTF8_Blended(
            nameFont, statusMessage.c_str(), errorColor);
        if (statusSurface) {
            SDL_Texture* statusTexture = SDL_CreateTextureFromSurface(
                renderer, statusSurface);
            if (statusTexture) {
                SDL_Rect destination = {
                    (screenW - statusSurface->w) / 2, screenH - 140,
                    statusSurface->w, statusSurface->h
                };
                SDL_RenderCopy(renderer, statusTexture, nullptr, &destination);
                SDL_DestroyTexture(statusTexture);
            }
            SDL_FreeSurface(statusSurface);
        }
    }
}
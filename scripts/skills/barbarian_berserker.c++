#include "scripts/skills/barbarian_berserker.h"
#include "scripts/characters/player.h"
#include "scripts/characters/combat_enemy.h"
#include "scripts/dice/dice_1d8.h"
#include "scripts/dice/dice_1d10.h"
#include "scripts/dice/dice_1d20.h"
#include <algorithm>
#include <cmath>

namespace barbarian_berserker {

player_save::Skill smash() {
    player_save::Skill skill;
    skill.name = "smash";
    skill.level = 1;
    skill.cooldown = 3;
    skill.description = "Powerful smash dealing 1d8 + Strength modifier damage.";
    skill.effect = "Launches 1d8 + Strength modifier against an enemy.";
    return skill;
}

player_save::Skill brutalCut() {
    player_save::Skill skill;
    skill.name = "brutal cut";
    skill.level = 1;
    skill.cooldown = 3;
    skill.description = "Deals 2d8 plus Strength modifier damage.";
    skill.effect = "On a successful attack, deals 2d8 + Strength modifier.";
    return skill;
}

player_save::Skill fierceBeat() {
    player_save::Skill skill;
    skill.name = "fierce beat";
    skill.level = 1;
    skill.cooldown = 5;
    skill.description =
        "Deals the higher of 2d10 plus Strength modifier damage.";
    skill.effect =
        "On a successful attack, deals the higher 2d10 + Strength modifier.";
    return skill;
}

void populateSkills(const std::string& characterClass,
                    const std::string& subclass,
                    std::vector<player_save::Skill>& skills) {
    if ((characterClass == "barbarian" || characterClass == "barbaro") &&
        (subclass == "path_berserker" || subclass == "berserker")) {
        skills.clear();
        skills.push_back(smash());
        skills.push_back(brutalCut());
        skills.push_back(fierceBeat());
    }
}

bool BattleSkillController::isAvailable(
    const Player& player, const std::string& skillName) const {
    if ((player.characterClass != "barbarian" && player.characterClass != "barbaro") ||
        (player.subclass != "path_berserker" && player.subclass != "berserker")) {
        return false;
    }
    return std::any_of(player.skills.begin(), player.skills.end(),
        [&player, &skillName](const player_save::Skill& skill) {
            return skill.name == skillName && skill.level <= player.level;
        });
}

std::string BattleSkillController::iconPath(
    const Player& player, const std::string& skillName) const {
    if (!isAvailable(player, skillName)) return {};
    const auto skill = std::find_if(
        player.skills.begin(), player.skills.end(),
        [&skillName](const player_save::Skill& candidate) {
            return candidate.name == skillName;
        });
    if (skill == player.skills.end()) return {};

    std::string iconName;
    if (skillName == "smash") {
        iconName = "smash";
    } else if (skillName == "brutal cut") {
        iconName = "brutal_cut";
    } else if (skillName == "fierce beat") {
        iconName = "fierce_beat";
    } else {
        return {};
    }
    return "resources/icons/skills/barbarian/berserker/level" +
           std::to_string(skill->level) + "_" + iconName + ".png";
}

int BattleSkillController::cooldownRemaining(const std::string& skillName) const {
    if (skillName == "smash") return smashCooldown;
    if (skillName == "brutal cut") return brutalCutCooldown;
    if (skillName == "fierce beat") return fierceBeatCooldown;
    return 0;
}

bool BattleSkillController::useSmash(
    Player& player, CombatEnemy& target, int& attackRoll, int& damage,
    int& damageDie, int& strengthModifier) {
    if (!isAvailable(player, "smash") || smashCooldown > 0 || !target.isAlive()) {
        return false;
    }

    strengthModifier = static_cast<int>(std::floor((player.strength - 10) / 2.0));
    attackRoll = Dice1d20::roll() + strengthModifier;
    damage = 0;
    damageDie = 0;

    if (attackRoll >= target.getArmorClass()) {
        damageDie = Dice1d8::roll();
        damage = std::max(1, damageDie + strengthModifier);
        target.takeDamage(damage);
    }

    const auto skill = std::find_if(
        player.skills.begin(), player.skills.end(),
        [](const player_save::Skill& candidate) {
            return candidate.name == "smash";
        });
    smashCooldown = (skill != player.skills.end() && skill->cooldown > 0)
                        ? skill->cooldown
                        : 3;
    return true;
}

bool BattleSkillController::useBrutalCut(
    Player& player, CombatEnemy& target, int& attackRoll, int& damage,
    int& firstDie, int& secondDie, int& strengthModifier) {
    if (!isAvailable(player, "brutal cut") ||
        brutalCutCooldown > 0 || !target.isAlive()) {
        return false;
    }

    strengthModifier =
        static_cast<int>(std::floor((player.strength - 10) / 2.0));
    attackRoll = Dice1d20::roll();
    firstDie = 0;
    secondDie = 0;
    damage = 0;
    if (attackRoll >= target.getArmorClass()) {
        firstDie = Dice1d8::roll();
        secondDie = Dice1d8::roll();
        damage = std::max(1, firstDie + secondDie + strengthModifier);
        target.takeDamage(damage);
    }

    const auto skill = std::find_if(
        player.skills.begin(), player.skills.end(),
        [](const player_save::Skill& candidate) {
            return candidate.name == "brutal cut";
        });
    brutalCutCooldown =
        skill != player.skills.end() && skill->cooldown > 0
            ? skill->cooldown : 3;
    return true;
}

bool BattleSkillController::useFierceBeat(
    Player& player, CombatEnemy& target, int& attackRoll, int& damage,
    int& firstDie, int& secondDie, int& discardedDie,
    int& strengthModifier) {
    if (!isAvailable(player, "fierce beat") ||
        fierceBeatCooldown > 0 || !target.isAlive()) {
        return false;
    }

    strengthModifier =
        static_cast<int>(std::floor((player.strength - 10) / 2.0));
    attackRoll = Dice1d20::roll();
    firstDie = 0;
    secondDie = 0;
    discardedDie = 0;
    damage = 0;
    if (attackRoll >= target.getArmorClass()) {
        firstDie = Dice1d10::roll();
        secondDie = Dice1d10::roll();
        const int keptDie = std::max(firstDie, secondDie);
        discardedDie = std::min(firstDie, secondDie);
        damage = std::max(1, keptDie + strengthModifier);
        target.takeDamage(damage);
    }

    const auto skill = std::find_if(
        player.skills.begin(), player.skills.end(),
        [](const player_save::Skill& candidate) {
            return candidate.name == "fierce beat";
        });
    fierceBeatCooldown =
        skill != player.skills.end() && skill->cooldown > 0
            ? skill->cooldown : 5;
    return true;
}

void BattleSkillController::finishPlayerTurn(Player& player, const std::string& usedSkill) {
    (void)player;
    if (usedSkill != "smash" && smashCooldown > 0) {
        --smashCooldown;
    }
    if (usedSkill != "brutal cut" && brutalCutCooldown > 0) {
        --brutalCutCooldown;
    }
    if (usedSkill != "fierce beat" && fierceBeatCooldown > 0) {
        --fierceBeatCooldown;
    }
}

void BattleSkillController::beginPlayerTurn(Player& player) {
    (void)player;
}

void BattleSkillController::endBattle(Player& player) {
    (void)player;
}

}
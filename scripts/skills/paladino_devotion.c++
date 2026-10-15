#include "scripts/skills/paladino_devotion.h"
#include "scripts/characters/player.h"
#include "scripts/characters/combat_enemy.h"
#include "scripts/dice/dice_1d8.h"
#include "scripts/dice/dice_1d20.h"
#include "scripts/wearpons/iron_hammer.h"
#include <algorithm>
#include <cmath>

namespace paladino_devotion {

namespace {

bool hasEquippedIronHammer(const Player& player) {
    return std::any_of(
        player.inventory.begin(), player.inventory.end(),
        [](const player_save::BagItem& item) {
            return item.name == IronHammer::name() &&
                   item.quantity > 0 && item.state == "equipado";
        });
}

}

player_save::Skill layingOnHands() {
    player_save::Skill skill;
    skill.name = "laying on of hands";
    skill.level = 1;
    skill.cooldown = 2;
    skill.description = "Restores 3 HP to the player.";
    skill.effect = "Immediately restores up to 3 HP to the player.";
    return skill;
}

player_save::Skill sacredWeapon() {
    player_save::Skill skill;
    skill.name = "sacred weapon";
    skill.level = 1;
    skill.cooldown = 5;
    skill.description = "increases Charisma by +1";
    skill.effect =
        "When used, it must increase the character's charisma for the next turn";
    return skill;
}

player_save::Skill deathBlow() {
    player_save::Skill skill;
    skill.name = "death blow";
    skill.level = 1;
    skill.cooldown = 2;
    skill.description =
        "A powerful weapon attack against an enemy.";
    skill.effect =
        "Uses the equipped weapon to attack an enemy.";
    return skill;
}

player_save::Skill crushingBlow() {
    player_save::Skill skill;
    skill.name = "crushing blow";
    skill.level = 1;
    skill.cooldown = 2;
    skill.description = "A powerful weapon attack against an enemy.";
    skill.effect =
        "Rolls 1d20 plus Strength against the target's Armor Class.";
    return skill;
}

player_save::Skill shieldProtection() {
    player_save::Skill skill;
    skill.name = "shield protection";
    skill.level = 1;
    skill.cooldown = 7;
    skill.description =
        "Protects the user from the opponent's next move on the next turn.";
    skill.effect =
        "If the opponent hits on its next turn, increases Armor Class by 3.";
    return skill;
}

void populateSkills(const std::string& characterClass,
                    const std::string& subclass,
                    std::vector<player_save::Skill>& skills) {
    skills.clear();
    if ((characterClass == "paladin" || characterClass == "paladino") &&
        subclass == "oath_devotion") {
        skills.push_back(layingOnHands());
        skills.push_back(sacredWeapon());
        skills.push_back(deathBlow());
        skills.push_back(crushingBlow());
        skills.push_back(shieldProtection());
    }
}

bool BattleSkillController::isAvailable(
    const Player& player, const std::string& skillName) const {
    if ((player.characterClass != "paladin" &&
         player.characterClass != "paladino") ||
        player.subclass != "oath_devotion") {
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
    if (skillName == "laying on of hands") {
        iconName = "laying_hands";
    } else if (skillName == "sacred weapon") {
        iconName = "sacred_weapon";
    } else if (skillName == "death blow") {
        iconName = "death_blow";
    } else if (skillName == "crushing blow") {
        iconName = "crushing_blow";
    } else if (skillName == "shield protection") {
        iconName = "shield_protection";
    } else {
        return {};
    }
    return "resources/icons/skills/paladino/devotion/level" +
           std::to_string(skill->level) + "_" + iconName + ".png";
}

int BattleSkillController::cooldownRemaining(
    const std::string& skillName) const {
    if (skillName == "laying on of hands") return layingOnHandsCooldown;
    if (skillName == "sacred weapon") return sacredWeaponCooldown;
    if (skillName == "death blow") return deathBlowCooldown;
    if (skillName == "crushing blow") return crushingBlowCooldown;
    if (skillName == "shield protection") return shieldProtectionCooldown;
    return 0;
}

bool BattleSkillController::useLayingOnHands(
    Player& player, int& healedAmount) {
    if (!isAvailable(player, "laying on of hands") ||
        layingOnHandsCooldown > 0 || player.hp >= player.maxHp) {
        return false;
    }
    const int previousHp = player.hp;
    player.hp = std::min(player.maxHp, player.hp + 3);
    healedAmount = player.hp - previousHp;
    const auto skill = std::find_if(
        player.skills.begin(), player.skills.end(),
        [](const player_save::Skill& candidate) {
            return candidate.name == "laying on of hands";
        });
    layingOnHandsCooldown =
        skill != player.skills.end() && skill->cooldown > 0
            ? skill->cooldown : 5;
    return true;
}

bool BattleSkillController::useSacredWeapon(Player& player) {
    if (!isAvailable(player, "sacred weapon") || sacredWeaponCooldown > 0) {
        return false;
    }
    const auto skill = std::find_if(
        player.skills.begin(), player.skills.end(),
        [](const player_save::Skill& candidate) {
            return candidate.name == "sacred weapon";
        });
    sacredWeaponCooldown =
        skill != player.skills.end() && skill->cooldown > 0
            ? skill->cooldown : 5;
    sacredWeaponPending = true;
    return true;
}

bool BattleSkillController::useDeathBlow(
    Player& player, CombatEnemy& target, int& attackRoll, int& damage,
    int& damageDie, int& strengthModifier) {
    if (!isAvailable(player, "death blow") ||
        deathBlowCooldown > 0 || !target.isAlive() ||
        !hasEquippedIronHammer(player)) {
        return false;
    }

    attackRoll = 0;
    const IronHammer::DamageRoll damageRoll =
        IronHammer::rollDamageDetailed(player);
    damageDie = damageRoll.dieRoll;
    strengthModifier = damageRoll.strengthModifier;
    damage = damageRoll.total;
    target.takeDamage(damage);

    const auto skill = std::find_if(
        player.skills.begin(), player.skills.end(),
        [](const player_save::Skill& candidate) {
            return candidate.name == "death blow";
        });
    deathBlowCooldown =
        skill != player.skills.end() && skill->cooldown > 0
            ? skill->cooldown : 2;
    return true;
}

bool BattleSkillController::useCrushingBlow(
    Player& player, CombatEnemy& target, int& attackRoll, int& damage,
    int& damageDie, int& strengthModifier) {
    if (!isAvailable(player, "crushing blow") ||
        crushingBlowCooldown > 0 || !target.isAlive() ||
        !hasEquippedIronHammer(player)) {
        return false;
    }

    attackRoll = Dice1d20::roll() +
        static_cast<int>(std::floor((player.strength - 10) / 2.0));
    damage = 0;
    damageDie = 0;
    strengthModifier = IronHammer::strengthModifier(player.strength);
    if (attackRoll >= target.getArmorClass()) {
        const IronHammer::DamageRoll damageRoll =
            IronHammer::rollDamageDetailed(player);
        damageDie = damageRoll.dieRoll;
        strengthModifier = damageRoll.strengthModifier;
        damage = damageRoll.total;
        target.takeDamage(damage);
    }

    const auto skill = std::find_if(
        player.skills.begin(), player.skills.end(),
        [](const player_save::Skill& candidate) {
            return candidate.name == "crushing blow";
        });
    crushingBlowCooldown =
        skill != player.skills.end() && skill->cooldown > 0
            ? skill->cooldown : 2;
    return true;
}

bool BattleSkillController::useShieldProtection(const Player& player) {
    if (!isAvailable(player, "shield protection") ||
        shieldProtectionCooldown > 0 || shieldProtectionActive) {
        return false;
    }
    const auto skill = std::find_if(
        player.skills.begin(), player.skills.end(),
        [](const player_save::Skill& candidate) {
            return candidate.name == "shield protection";
        });
    shieldProtectionCooldown =
        skill != player.skills.end() && skill->cooldown > 0
            ? skill->cooldown : 7;
    shieldProtectionActive = true;
    return true;
}

bool BattleSkillController::shieldProtectionPending() const {
    return shieldProtectionActive;
}

void BattleSkillController::resolveShieldProtection() {
    shieldProtectionActive = false;
}

void BattleSkillController::finishPlayerTurn(
    Player& player, const std::string& usedSkill) {
    if (sacredWeaponActive) {
        --player.charisma;
        sacredWeaponActive = false;
    }
    if (usedSkill != "laying on of hands" && layingOnHandsCooldown > 0) {
        --layingOnHandsCooldown;
    }
    if (usedSkill != "sacred weapon" && sacredWeaponCooldown > 0) {
        --sacredWeaponCooldown;
    }
    if (usedSkill != "death blow" && deathBlowCooldown > 0) {
        --deathBlowCooldown;
    }
    if (usedSkill != "crushing blow" && crushingBlowCooldown > 0) {
        --crushingBlowCooldown;
    }
    if (usedSkill != "shield protection" && shieldProtectionCooldown > 0) {
        --shieldProtectionCooldown;
    }
}

void BattleSkillController::beginPlayerTurn(Player& player) {
    if (sacredWeaponPending) {
        ++player.charisma;
        sacredWeaponActive = true;
        sacredWeaponPending = false;
    }
}

void BattleSkillController::endBattle(Player& player) {
    if (sacredWeaponActive) {
        --player.charisma;
        sacredWeaponActive = false;
    }
    sacredWeaponPending = false;
}

}

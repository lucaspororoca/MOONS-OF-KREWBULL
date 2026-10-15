#ifndef PALADINO_DEVOTION_H
#define PALADINO_DEVOTION_H

#include "scripts/resources/player_save.h"
#include <string>
#include <vector>

class Player;
class CombatEnemy;

namespace paladino_devotion {

player_save::Skill layingOnHands();
player_save::Skill sacredWeapon();
player_save::Skill deathBlow();
player_save::Skill crushingBlow();
player_save::Skill shieldProtection();
void populateSkills(const std::string& characterClass,
                    const std::string& subclass,
                    std::vector<player_save::Skill>& skills);
class BattleSkillController {
public:
    bool isAvailable(const Player& player, const std::string& skillName) const;
    std::string iconPath(const Player& player,
                         const std::string& skillName) const;
    int cooldownRemaining(const std::string& skillName) const;
    bool useLayingOnHands(Player& player, int& healedAmount);
    bool useSacredWeapon(Player& player);
    bool useDeathBlow(Player& player, CombatEnemy& target,
                      int& attackRoll, int& damage,
                      int& damageDie, int& strengthModifier);
    bool useCrushingBlow(Player& player, CombatEnemy& target,
                         int& attackRoll, int& damage,
                         int& damageDie, int& strengthModifier);
    bool useShieldProtection(const Player& player);
    bool shieldProtectionPending() const;
    void resolveShieldProtection();
    void finishPlayerTurn(Player& player, const std::string& usedSkill);
    void beginPlayerTurn(Player& player);
    void endBattle(Player& player);

private:
    int layingOnHandsCooldown = 0;
    int sacredWeaponCooldown = 0;
    int deathBlowCooldown = 0;
    int crushingBlowCooldown = 0;
    int shieldProtectionCooldown = 0;
    bool sacredWeaponPending = false;
    bool sacredWeaponActive = false;
    bool shieldProtectionActive = false;
};

}

#endif

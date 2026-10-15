#ifndef BARBARIAN_BERSERKER_H
#define BARBARIAN_BERSERKER_H

#include "scripts/resources/player_save.h"
#include <string>
#include <vector>

class Player;
class CombatEnemy;

namespace barbarian_berserker {

player_save::Skill smash();
player_save::Skill brutalCut();
player_save::Skill fierceBeat();

void populateSkills(const std::string& characterClass,
                    const std::string& subclass,
                    std::vector<player_save::Skill>& skills);

class BattleSkillController {
public:
    bool isAvailable(const Player& player, const std::string& skillName) const;
    std::string iconPath(const Player& player, const std::string& skillName) const;
    int cooldownRemaining(const std::string& skillName) const;
    bool useSmash(Player& player, CombatEnemy& target, int& attackRoll,
                  int& damage, int& damageDie, int& strengthModifier);
    bool useBrutalCut(Player& player, CombatEnemy& target, int& attackRoll,
                      int& damage, int& firstDie, int& secondDie,
                      int& strengthModifier);
    bool useFierceBeat(Player& player, CombatEnemy& target, int& attackRoll,
                       int& damage, int& firstDie, int& secondDie,
                       int& discardedDie, int& strengthModifier);
    void finishPlayerTurn(Player& player, const std::string& usedSkill);
    void beginPlayerTurn(Player& player);
    void endBattle(Player& player);

private:
    int smashCooldown = 0;
    int brutalCutCooldown = 0;
    int fierceBeatCooldown = 0;
};

}

#endif
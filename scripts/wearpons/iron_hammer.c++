#include "scripts/wearpons/iron_hammer.h"

#include "scripts/characters/player.h"
#include "scripts/dice/dice_1d8.h"
#include <algorithm>
#include <cmath>

const std::string& IronHammer::name() {
    static const std::string value = "Iron Hammer";
    return value;
}

const std::string& IronHammer::weaponType() {
    static const std::string value = "arma pesada";
    return value;
}

const std::string& IronHammer::style() {
    static const std::string value = "1 mão";
    return value;
}

int IronHammer::strengthModifier(int strength) {
    return static_cast<int>(std::floor((strength - 10) / 2.0));
}

int IronHammer::rollDamage(int strength) {
    return rollDamageDetailed(strength).total;
}

int IronHammer::rollDamage(const Player& player) {
    return rollDamage(player.strength);
}

IronHammer::DamageRoll IronHammer::rollDamageDetailed(int strength) {
    DamageRoll result;
    result.dieRoll = Dice1d8::roll();
    result.strengthModifier = strengthModifier(strength);
    result.total = std::max(1, result.dieRoll + result.strengthModifier);
    return result;
}

IronHammer::DamageRoll IronHammer::rollDamageDetailed(const Player& player) {
    return rollDamageDetailed(player.strength);
}

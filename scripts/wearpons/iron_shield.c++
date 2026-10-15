#include "scripts/wearpons/iron_shield.h"

#include "scripts/armors/plate_armor.h"
#include "scripts/characters/player.h"
#include <algorithm>
#include <cmath>

const std::string& IronShield::name() {
    static const std::string value = "Iron Shield";
    return value;
}

const std::string& IronShield::style() {
    static const std::string value = "1 mão";
    return value;
}

int IronShield::armorClassFor(const Player& player) {
    int dexterityModifier =
        static_cast<int>(std::floor((player.dexterity - 10) / 2.0));
    const bool armorEquipped = std::any_of(
        player.inventory.begin(), player.inventory.end(),
        [](const player_save::BagItem& item) {
            return item.name == PlateArmor::name() &&
                   item.state == "equipado" && item.quantity > 0;
        });

    const bool shieldEquipped = std::any_of(
        player.inventory.begin(), player.inventory.end(),
        [](const player_save::BagItem& item) {
            return item.name == "Iron Shield" &&
                   item.state == "equipado" &&
                   item.quantity > 0;
        });
    return (armorEquipped ? PlateArmor::armorClassBonus() : 10) +
           dexterityModifier +
           (shieldEquipped ? armorClassBonus() : 0);
}

#ifndef IRON_SHIELD_H
#define IRON_SHIELD_H

#include <string>

class Player;

class IronShield {
public:
    static const std::string& name();
    static constexpr int durability() { return 50; }
    static constexpr float weight() { return 25.0f; }
    static const std::string& style();
    static constexpr int armorClassBonus() { return 1; }
    static int armorClassFor(const Player& player);
};

#endif

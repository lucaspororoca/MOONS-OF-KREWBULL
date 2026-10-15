#ifndef IRON_HAMMER_H
#define IRON_HAMMER_H

#include <string>

class Player;

class IronHammer {
public:
    struct DamageRoll {
        int dieRoll = 0;
        int strengthModifier = 0;
        int total = 0;
    };

    static const std::string& name();
    static constexpr int durability() { return 25; }
    static constexpr float weight() { return 16.0f; }
    static constexpr int saleValue() { return 65; }
    static const std::string& weaponType();
    static const std::string& style();
    static int strengthModifier(int strength);
    static int rollDamage(int strength);
    static int rollDamage(const Player& player);
    static DamageRoll rollDamageDetailed(int strength);
    static DamageRoll rollDamageDetailed(const Player& player);
};

#endif

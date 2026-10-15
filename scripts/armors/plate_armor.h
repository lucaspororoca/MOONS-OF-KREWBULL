#ifndef PLATE_ARMOR_H
#define PLATE_ARMOR_H

#include <string>

class PlateArmor {
public:
    static const std::string& name();
    static constexpr int durability() { return 30; }
    static constexpr int armorClassBonus() { return 10; }
    static constexpr float weight() { return 32.0f; }
    static constexpr int saleValue() { return 25; }
};

#endif

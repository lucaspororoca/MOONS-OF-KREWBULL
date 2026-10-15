#include "scripts/dice/dice_1d6.h"

#include <algorithm>
#include <array>
#include <random>

namespace {
std::mt19937& randomEngine() {
    static std::mt19937 engine(std::random_device{}());
    return engine;
}
}

int Dice1d6::roll() {
    static std::uniform_int_distribution<int> distribution(1, 6);
    return distribution(randomEngine());
}

int Dice1d6::roll4d6DropLowest() {
    std::array<int, 4> rolls = {{roll(), roll(), roll(), roll()}};
    std::sort(rolls.begin(), rolls.end());
    return rolls[1] + rolls[2] + rolls[3];
}

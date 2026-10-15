#include "scripts/dice/dice_1d8.h"

#include <random>

int Dice1d8::roll() {
    static std::mt19937 engine(std::random_device{}());
    static std::uniform_int_distribution<int> distribution(1, 8);
    return distribution(engine);
}

#include "scripts/dice/dice_1d12.h"
#include <random>

int Dice1d12::roll() {
    static std::mt19937 engine(std::random_device{}());
    static std::uniform_int_distribution<int> distribution(1, 12);
    return distribution(engine);
}

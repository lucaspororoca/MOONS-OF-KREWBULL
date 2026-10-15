#include "scripts/dice/dice_1d10.h"
#include <random>

int Dice1d10::roll() {
    static std::mt19937 engine(std::random_device{}());
    static std::uniform_int_distribution<int> distribution(1, 10);
    return distribution(engine);
}

#include "scripts/dice/dice_1d20.h"
#include <cstdlib>
#include <ctime>

static bool seeded = false;

int Dice1d20::roll() {
    if (!seeded) {
        std::srand(static_cast<unsigned>(std::time(nullptr)));
        seeded = true;
    }
    return (std::rand() % 20) + 1;
}
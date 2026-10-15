#include "scripts/dice/dice_1d4.h"
#include <cstdlib>
#include <ctime>

static bool d4Seeded = false;

int Dice1d4::roll() {
    if (!d4Seeded) {
        std::srand(static_cast<unsigned>(std::time(nullptr)));
        d4Seeded = true;
    }
    return (std::rand() % 4) + 1;
}
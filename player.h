#pragma once

#include <stdint.h>
#include <string>
#include "math.h"

class Player {
   public:
    Player(float xx, float yy, int direction, std::string name)
        : x(xx), y(yy), direction(direction), name(name) {}

    std::string name;
    uint8_t number;

    bool active = false;
    int clientIndex = -1;

    int direction;
    float x;
    float y;

    uint32_t x() { return floor(x); }
    uint32_t y() { return floor(y); }

    bool operator<(const PLayer& p1, const Player& p2) {
        return p1.name < p2.name;
    }
}
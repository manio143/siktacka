#pragma once

#include <stdint.h>
#include <string>
#include "math.h"

class Player {
   private:
    float _x;
    float _y;

   public:
    Player(float x, float y, int direction, std::string name)
        : _x(x), _y(y), direction(direction), name(name) {}

    std::string name;
    uint8_t number;

    bool active = false;
    int clientIndex = -1;

    int direction;

    void setX(float x) { _x = x;}
    void setY(float y) { _y = y;}

    uint32_t x() { return floor(_x); }
    uint32_t y() { return floor(_y); }

    bool operator<(const Player& p2) {
        return name < p2.name;
    }
};
#pragma once

#include <stdint.h>
#include <cassert>

#include "player.h"
#include "err.h"

class PixelBoard {
   private:
    uint32_t _width, _height;
    char* pixels;
    char outOfBoundsValue = 1;

    char& getPixel(Player& player) {
        outOfBoundsValue = -1;
        auto x = player.x();
        auto y = player.y();
        if (!(x >= 0 && x < _width && y >= 0 && y < _height)) {
            debug("Player out of bounds (%d,%d)\n", x, y);
            return outOfBoundsValue;
        }
        return pixels[y * _width + x];
    }

   public:
    PixelBoard(uint32_t width, uint32_t height)
        : _width(width), _height(height), pixels(0) {
        pixels = new char[width * height];
        memset(pixels, 0, width * height);
    }

    void set(Player& player) { getPixel(player) = player.number + 1; }

    bool isSet(Player& player) {
        auto pixel = getPixel(player);
        return pixel != 0;
    }

    ~PixelBoard() { delete[] pixels; }

    PixelBoard& operator=(PixelBoard&& other) {
        if (this != &other) {
            this->_width = other._width;
            this->_height = other._height;
            auto pix = other.pixels;
            other.pixels = this->pixels;
            this->pixels = pix;
        }
        return *this;
    }
};
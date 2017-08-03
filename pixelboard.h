#pragma once

#include <stdint.h>
#include <cassert>

#include "player.h"

class PixelBoard {
   private:
    uint32_t _width, _height;
    char* pixels;

    char& getPixel(Player& player) {
        auto x = player.x();
        auto y = player.y();
        assert(x >= 0 && x <_width && y >= 0 && y < _height);
        return pixels[y * _width + x];
    }

   public:
    PixelBoard(uint32_t width, uint32_t height)
        : _width(width), _height(height), pixels(0) {
        pixels = new char[width * height];
        memset(pixels, 0, width * height);
    }

    void set(Player& player) { getPixel(player) = player.number + 1; }

    bool isSetAndNot(Player& player) {
        auto pixel = getPixel(player);
        return pixel != 0 && pixel != player.number + 1;
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
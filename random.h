#pragma once

#include <stdint.h>

class Random {
   private:
    uint64_t _seed;
    uint64_t _value;

   public:
    Random(uint64_t seed) : _seed(seed), _value(0) {}

    uint64_t next() {
        _value = _seed;
        _seed = (_seed * 279470273) % 4294967291;
        return _value;
    }
};
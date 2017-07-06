#pragma once

#include <stdint.h>
#include <string>

#include "ipaddress.h"

class Client {
   public:
    Client(/**/) {}

    std::string playerName;
    IPAddress ip;
    int turnDirection;
    uint64_t time;

    int playerIndex;
}
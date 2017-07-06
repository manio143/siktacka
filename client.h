#pragma once

#include <stdint.h>
#include <string>
#include <time.h>

#include "ipaddress.h"

class Client {
   public:
    Client(uint64_t sessionId, int turnDirection, std::string playerName)
        : sessionId(sessionId),
          turnDirection(turnDirection),
          playerName(playerName),
          time(::time(NULL)) {
    }

    std::string playerName;
    IPAddress ip;
    int turnDirection;
    uint64_t time;
    uint64_t sessionId;

    int playerIndex = -1;

    bool operator==(const Client& c2) {
        return sessionId == c2.sessionId && playerName == c2.playerName &&
               ip == c2.ip;
    }
};
#pragma once

#include <stdint.h>
#include <string>
#include <memory>
#include "events.h"

class ClientMessage {
   private:
    uint64_t _sessionId;
    uint8_t _turnDirection;
    uint32_t _nextExpectedEventNumber;
    std::string _playerName;

   public:
    ClientMessage(uint64_t sessionId,
                  uint8_t turnDirection,
                  uint32_t nextExpectedEventNumber,
                  std::string playerName)
        : _sessionId(sessionId),
          _turnDirection(turnDirection),
          _nextExpectedEventNumber(nextExpectedEventNumber),
          _playerName(playerName) {}

    uint64_t sessionId() { return _sessionId; }
    uint8_t turnDirection() { return _turnDirection; }
    uint32_t nextExpectedEventNumber() { return _nextExpectedEventNumber; }
    std::string playerName() { return _playerName; }

    void* serialize(void* buffer);
    static void* deserialize(void* buffer, std::shared_ptr<ClientMessage> & message);
}

class ServerMessage {
   private:
    uint32_t _gameId;
    vector<std::shared_ptr<Event>> _events;

   public:
    ServerMessage(uint32_t gameId, vector<std::shared_ptr<Event>> events)
        : _gameId(gameId), _events(events) {}
    
    uint32_t gameId() { return _gameId; }
    vector<std::shared_ptr<Event>> events() { return _events; }
    
    void* serialize(void* buffer);
    static void* deserialize(void* buffer, std::shared_ptr<ServerMessage> & message);
}
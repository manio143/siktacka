#pragma once

#include <unistd.h>
#include <stdint.h>
#include <string>
#include <memory>

#include "const.h"
#include "binwriter.h"

class Event {
   private:
    uint32_t _number;
    uint8_t _type;

   protected:
    Event(uint32_t number, uint8_t type) : _number(number), _type(type) {}
    virtual void serializeData(BinaryWriter& bw) = 0;

   public:
    uint32_t number() { return _number; }
    uint8_t type() { return _type; }

    virtual uint32_t len() { return sizeof(_number) + sizeof(_type); }
    uint32_t totalLen() { return len() + 2 * sizeof(uint32_t); }
    char* serialize(char* buffer);
    virtual size_t toString(char* buffer) = 0;

    virtual ~Event() {}
};

class NewGameEvent : public Event {
   private:
    uint32_t _maxx, _maxy;
    std::string _playerNames;

   protected:
    virtual void serializeData(BinaryWriter& bw) override;

   public:
    NewGameEvent(uint32_t number,
                 uint32_t maxx,
                 uint32_t maxy,
                 std::string playerNames)
        : Event(number, NEW_GAME),
          _maxx(maxx),
          _maxy(maxy),
          _playerNames(playerNames) {}

    NewGameEvent(uint32_t number,
                 uint32_t maxx,
                 uint32_t maxy,
                 std::vector<std::string>& names)
        : Event(number, NEW_GAME), _maxx(maxx), _maxy(maxy), _playerNames("") {
        for (auto& name : names) {
            _playerNames.append(name);
            _playerNames.push_back(0);
        }
    }

    uint32_t maxx() { return _maxx; }
    uint32_t maxy() { return _maxy; }
    std::string playerNames() { return _playerNames; }

    virtual uint32_t len() {
        return Event::len() + sizeof(_maxx) + sizeof(_maxy) +
               _playerNames.size();
    }

    virtual size_t toString(char* buffer);
};

class PixelEvent : public Event {
   private:
    uint8_t _playerNumber;
    uint32_t _x, _y;

   protected:
    virtual void serializeData(BinaryWriter& bw) override;

   public:
    PixelEvent(uint32_t number, uint8_t playerNumber, uint32_t x, uint32_t y)
        : Event(number, PIXEL), _playerNumber(playerNumber), _x(x), _y(y) {}

    uint8_t playerNumber() { return _playerNumber; }
    uint32_t x() { return _x; }
    uint32_t y() { return _y; }

    virtual uint32_t len() {
        return Event::len() + sizeof(_playerNumber) + sizeof(_x) + sizeof(_y);
    }
    virtual size_t toString(char* buffer);
};

class PlayerEliminatedEvent : public Event {
   private:
    uint8_t _playerNumber;

   protected:
    virtual void serializeData(BinaryWriter& bw) override;

   public:
    PlayerEliminatedEvent(uint32_t number, uint8_t playerNumber)
        : Event(number, PLAYER_ELIMINATED), _playerNumber(playerNumber) {}

    uint8_t playerNumber() { return _playerNumber; }

    virtual uint32_t len() { return Event::len() + sizeof(_playerNumber); }
    virtual size_t toString(char* buffer);
};

class GameOverEvent : public Event {
   protected:
    virtual void serializeData(BinaryWriter& bw) override {}

   public:
    GameOverEvent(uint32_t number) : Event(number, GAME_OVER) {}
    virtual size_t toString(char* buffer) { return 0; }
};

class EventDeserializer {
   public:
    static char* deserialize(char* buff, std::shared_ptr<Event>* event);
};

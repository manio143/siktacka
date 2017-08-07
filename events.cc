#include <algorithm>

#include "events.h"
#include "binwriter.h"
#include "const.h"

char* Event::serialize(char* buffer) {
    BinaryWriter bw;

    bw.write32(this->len());
    bw.write32(this->number());
    bw.write8(this->type());

    this->serializeData(bw);

    bw.writeCRC();

    return bw.copyTo(buffer);
}

void NewGameEvent::serializeData(BinaryWriter& bw) {
    bw.write32(this->maxx());
    bw.write32(this->maxy());
    bw.writeString(this->playerNames());
}

void PixelEvent::serializeData(BinaryWriter& bw) {
    bw.write8(this->playerNumber());
    bw.write32(this->x());
    bw.write32(this->y());
}

void PlayerEliminatedEvent::serializeData(BinaryWriter& bw) {
    bw.write8(this->playerNumber());
}

char* EventDeserializer::deserialize(char* buff,
                                     std::shared_ptr<Event>* event) {
    BinaryReader br(buff, sizeof(uint32_t));
    auto len = br.read32();

    if (len == 0)
        return buff + MAX_PACKET_SIZE;

    br = BinaryReader(buff, len + 2 * sizeof(uint32_t));
    br.read32();  // move past the len

    auto number = br.read32();
    auto type = br.read8();

    if (!br.checkCRC())
        return buff + MAX_PACKET_SIZE;

    switch (type) {
        case NEW_GAME: {
            auto maxx = br.read32();
            auto maxy = br.read32();
            size_t stringLen = len - 2 * sizeof(uint32_t) /*maxx i maxxy*/ - sizeof(uint32_t) /* event_no */ - sizeof(uint8_t) /* event_type */;
            *event = std::make_shared<NewGameEvent>(
                number, maxx, maxy, br.readString(stringLen));
        } break;
        case PIXEL: {
            auto playerNumber = br.read8();
            auto x = br.read32();
            auto y = br.read32();
            *event = std::make_shared<PixelEvent>(number, playerNumber, x, y);
        } break;
        case PLAYER_ELIMINATED:
            *event =
                std::make_shared<PlayerEliminatedEvent>(number, br.read8());
            break;
        case GAME_OVER:
            *event = std::make_shared<GameOverEvent>(number);
            break;
    }

    return buff + len + 2 * sizeof(uint32_t);
}

size_t NewGameEvent::toString(char* buffer) {
    std::replace(this->_playerNames.begin(), this->_playerNames.end(), 0, (int)' ');
    return sprintf(buffer, "NEW_GAME %d %d %s\n", this->maxx(), this->maxy(),
                   this->_playerNames.c_str());
}
size_t PixelEvent::toString(char* buffer) {
    return sprintf(buffer, "PIXEL %d %d ", this->x(), this->y());
}
size_t PlayerEliminatedEvent::toString(char* buffer) {
    return sprintf(buffer, "PLAYER_ELIMINATED ");
}

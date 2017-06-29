#include "events.h"
#include "binwriter.h"

void * Event::serialize(void * buffer) {
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
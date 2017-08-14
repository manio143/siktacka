#include "messages.h"
#include "exception.h"

size_t ClientMessage::serialize(char* buffer) {
    BinaryWriter bw;

    bw.write64(_sessionId);
    bw.write8(_turnDirection);
    bw.write32(_nextExpectedEventNumber);
    bw.writeString(_playerName);

    bw.copyTo(buffer);
    return bw.size();
}

bool invalidNameString(std::string s) {
    if(s.length() > 64)
        return true;
    for(auto c : s)
        if(c < 33 || c > 126)
            return true;
    return false;
}

void ClientMessage::deserialize(char* buffer,
                                std::shared_ptr<ClientMessage>* message) {
    BinaryReader br(buffer, MAX_PACKET_SIZE);

    auto sessionId = br.read64();
    int8_t turnDirection = br.read8();
    auto nextEEN = br.read32();
    auto playerName = br.readNullString();

    if(turnDirection > 1 || turnDirection < -1)
        throw InvalidValueException("turnDirection");
    if(invalidNameString(playerName))
        throw InvalidValueException("playerName");

    *message = std::make_shared<ClientMessage>(sessionId, turnDirection, nextEEN,
                                          playerName);
}

size_t ServerMessage::serialize(char* buffer) {
    char* bufferBegin = buffer;
    *((uint32_t*)buffer) = gameId();
    buffer += sizeof(uint32_t);

    for (auto& ev_ptr : _events)
        buffer = ev_ptr->serialize(buffer);

    return buffer - bufferBegin;
}

void ServerMessage::deserialize(char* buffer,
                                std::shared_ptr<ServerMessage>* message) {
    char* bufferBegin = buffer;
    auto gameId = *((uint32_t*)buffer);
    buffer += sizeof(uint32_t);

    std::vector<std::shared_ptr<Event>> events;

    while (buffer - bufferBegin <= MAX_PACKET_SIZE) {
        std::shared_ptr<Event> ev_ptr;
        buffer = EventDeserializer::deserialize(buffer, &ev_ptr);
        if (ev_ptr)
            events.push_back(ev_ptr);
    }

    *message = std::make_shared<ServerMessage>(gameId, events);
}
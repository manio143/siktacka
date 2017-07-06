#include "messages.h"

size_t ClientMessage::serialize(void* buffer) {
    BinaryWriter bw;

    bw.write64(_sessionId);
    bw.write8(_turnDirection);
    bw.write32(_nextExpectedEventNumber);
    bw.writeString(_playerName);

    bw.copyTo(buffer);
    return bw.size();
}

void ClientMessage::deserialize(void* buffer,
                                std::shared_ptr<ClientMessage>* message) {
    BinaryReader br(buffer, MAX_PACKET_SIZE);

    auto sessionId = br.read64();
    auto turnDirection = br.read8();
    auto nextEEN = br.read32();
    auto playerName = br.readNullString();

    *message = std::make_shared<ClientMessage>(sessionId, turnDirection, nextEEN,
                                          playerName);
}

size_t ServerMessage::serialize(void* buffer) {
    void* bufferBegin = buffer;
    *((uint32_t*)buffer) = gameId();
    buffer += sizeof(uint32_t);

    for (auto& ev_ptr : _events)
        buffer = ev_ptr->serialize(buffer);

    return buffer - bufferBegin;
}

void ServerMessage::deserialize(void* buffer,
                                std::shared_ptr<ServerMessage>* message) {
    void* bufferBegin = buffer;
    auto gameId = *((uint32_t*)buffer);
    buffer += sizeof(uint32_t);

    vector<std::shared_ptr<Event>> events;

    while (buffer - bufferBegin <= MAX_PACKET_SIZE) {
        std::shared_ptr<Event> ev_ptr;
        buffer = EventDeserializer::deserialize(buffer, &ev_ptr);
        if (ev_ptr)
            events.push_back(ev_ptr);
    }

    *message = std::make_shared<ServerMessage>(gameId, events);
}
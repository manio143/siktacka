// Main application

#include <vector>
#include <memory>
#include <sstream>
#include <iostream>
#include <time.h>

#include "sock.h"
#include "messages.h"
#include "events.h"
#include "connectors.h"
#include "arguments.h"
#include "const.h"

using EventsContainer = std::vector<std::shared_ptr<Event>>;

class Client {
   private:
    uint8_t _turnDirection = 0;
    int _lastEventSentToGui = -1;
    uint64_t _sessionId;
    bool _running = false;
    int _currentGameId;

    EventsContainer _events;
    ClientArguments _arguments;
    Sock _serverSock;
    Sock _guiSock;
    std::vector<std::string> _players;

    int nextExpectedEventNumber();
    bool checkForIncomingPackets(Sock& sock);
    void setPlayerNames(std::string names);
    void sendRequestToServer();
    void receiveFromServer();
    void processEvents(EventContainer& events);
    void sendEventsToGui();
    void receiveTurnDirection();
    bool every20ms();

    char* buffer();

   public:
    Client(int argc, char** argv)
        : _arguments(argc, argv), _sessionId(time(NULL)) {
        _serverSock = UdpConnector::connectTo(_arguments.host, _arguments.port);
        _guiSock =
            TcpConnector::connectTo(_arguments.guihost, _arguments.guiport);
        _sessionId = time(NULL);
    }

    void run();
}

char* Client::buffer() {
    static vector<char> v(MAX_PACKET_SIZE);
    memset(&v[0], 0, v.size());
    return &v[0];
}

int Client::nextExpectedEventNumber() {
    if (_events.size() > 0)
        return _events.back().number() + 1;
    else
        return 0;
}

void Client::sendRequestToServer() {
    char* buffer = this->buffer();
    ClientMessage message(_sessionId, _turnDirection, nextExpectedEventNumber(),
                          _arguments.playerName);
    int len = message.serialize(buffer);
    _serverSock.write(buffer, len);
}

void Client::setPlayerNames(std::string names) {
    std::istringstream f(names);
    std::string line;
    while (std::getline(f, line, 0))
        _players.push_back(line);
}

void Client::processEvents(EventContainer& events) {
    for (auto& event : events) {
        if (_events.size() == 0 && event->type() == NEW_GAME) {
            _events.push_back(event);
            _running = true;
            setPlayerNames(static_cast<NewGameEvent>(event).playerNames());
        } else if (event->number() == _events.back().number() + 1)
            _events.push_back(event);
    }

    if (_lastEventSentToGui == _events.back().number() &&
        _events.back().type() == GAME_OVER) {
        _events.clear();
        _running = false;
    }
}

bool Client::checkForIncomingPackets(Sock& sock) {
    auto p = sock.poll();
    if (p == Success)
        return true;
    if (p == Timeout)
        return false;
    else
        err("poll encountered an error.\n");
}

void Client::receiveFromServer() {
    if (!checkForIncomingPackets(_serverSock))
        return;

    char* buffer = this->buffer();

    _serverSock.read(buffer, MAX_PACKET_SIZE);

    std::shared_ptr<ServerMessage> msg_ptr;
    ServerMessage::deserialize(buffer, &msg_ptr);

    if (msg_ptr->gameId() != _currentGameId && _running)
        return;

    _currentGameId = msg_ptr->gameId();

    processEvents(msg_ptr->events());
}

void Client::sendEventsToGui() {
    if (!_running)
        return;

    while (_lastEventSentToGui < _events.back().number()) {
        _lastEventSentToGui++;
        auto& event = _events[_lastEventSentToGui];
        char* buffer = this->buffer();
        auto len = event->toString(buffer);

        if (event->type() == PIXEL)
            len += sprintf(
                buffer + len, "%s\n",
                _players[static_cast<PixelEvent>(*event).playerNumber()]);
        if (event->type() == PLAYER_ELIMINATED))
            len += sprintf(buffer + len, "%s\n", _players[static_cast<PlayerEliminatedEvent>(*event).playerNumber()]);

        _guiSock.write(buffer, len);
    }
}

void Client::receiveTurnDirection() {
    if (!checkForIncomingPackets(_guiSock))
        return;

    char* buffer = this->buffer();
    _guiSock.read(buffer, MAX_PACKET_SIZE);

    std::istringstream f(buffer);
    std::string line;
    while (std::getline(f, line)) {
        if (line.compare("LEFT_KEY_DOWN"))
            _turnDirection = -1;
        else if (line.compare("LEFT_KEY_UP"))
            _turnDirection = 0;
        if (line.compare("RIGHT_KEY_DOWN"))
            _turnDirection = 1;
        if (line.compare("RIGHT_KEY_UP"))
            _turnDirection = 0;
    }
}

bool Client::every20ms() {
    static struct timespec prev = {0, 0};
    static struct timespec next = {0, 0};

    if (clock_gettime(CLOCK_MONOTONIC, &next))
        err("clock_gettime");
    if (next.tv_sec > prev.tv_sec ||
        next.tv_nsec - prev.tv_nsec > 20000000) {  // 20ms
        prev = next;
        return true;
    }
    return false;
}

void Client::run() {
    while (true) {
        if (every20ms())
            sendRequestToServer();
        receiveFromServer();
        sendEventsToGui();
        receiveTurnDirection();
    }
}

int main(int argc, char** argv) {
    Client client(argc, argv);
    client.run();
    return 0;
}
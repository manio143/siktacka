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
#include "err.h"

using EventsContainer = std::vector<std::shared_ptr<Event>>;

class Client {
   private:
    uint8_t _turnDirection = 0;
    int _lastEventSentToGui = -1;
    uint64_t _sessionId;
    bool _running = false;
    bool _reset = false;
    uint32_t _currentGameId;
    int _maxx = 0, _maxy = 0;

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
    void validateEvent(std::shared_ptr<Event>& event);
    void processEvents(EventsContainer& events);
    void sendEventsToGui();
    void receiveTurnDirection();
    bool every20ms();
    void checkForGameOver();

    char* buffer();

   public:
    Client(int argc, char** argv)
        : _arguments(argc, argv),
          _sessionId(time(NULL)),
          _serverSock(0),
          _guiSock(0) {
        _serverSock = UdpConnector::connectTo(_arguments.host, _arguments.port);
        _guiSock =
            TcpConnector::connectTo(_arguments.guihost, _arguments.guiport);
    }

    void run();
};

char* Client::buffer() {
    static std::vector<char> v(MAX_PACKET_SIZE);
    memset(&v[0], 0, v.size());
    return &v[0];
}

int Client::nextExpectedEventNumber() {
    if (_events.size() > 0)
        return _events.back()->number() + 1;
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
    while (std::getline(f, line, (char)0))
        _players.push_back(line);
}

void Client::validateEvent(std::shared_ptr<Event>& event) {
    if (event->type() == PIXEL) {
        auto ev = static_cast<PixelEvent*>(event.get());
        if (ev->x() > _maxx || ev->y() > _maxy ||
            ev->playerNumber() > _players.size())
            err("Invalid data in PIXEL event.\n");
    } else if (event->type() == PLAYER_ELIMINATED) {
        auto ev = static_cast<PlayerEliminatedEvent*>(event.get());
        if (ev->playerNumber() > _players.size())
            err("Invalid data in PLAYER_ELIMINATED event.\n");
    }
}

void Client::processEvents(EventsContainer& events) {
    for (auto& event : events) {
        if (_events.size() == 0 && event->type() == NEW_GAME) {
            _events.push_back(event);
            _running = true;
            setPlayerNames(
                static_cast<NewGameEvent*>(event.get())->playerNames());
        } else if (_events.size() == 0) {
            break;
        } else if (event->number() == _events.back()->number() + 1) {
            validateEvent(event);
            _events.push_back(event);
        }
    }
}

void Client::checkForGameOver() {
    if (_events.size() > 0 && _lastEventSentToGui == _events.back()->number() &&
        _events.back()->type() == GAME_OVER) {
        _events.clear();
        _running = false;
        _reset = true;
        _lastEventSentToGui = -1;
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
    return false;
}

void Client::receiveFromServer() {
    if (!checkForIncomingPackets(_serverSock))
        return;

    char* buffer = this->buffer();

    _serverSock.read(buffer, MAX_PACKET_SIZE);

    std::shared_ptr<ServerMessage> msg_ptr;
    try {
        ServerMessage::deserialize(buffer, &msg_ptr);
    } catch (std::exception ex) {
        err("An error occured while deserializing the message from server.\n");
    }

    if (msg_ptr->gameId() != _currentGameId && _running)
        return;
    if (msg_ptr->gameId() == _currentGameId && _reset)
        return;

    _reset = false;
    _currentGameId = msg_ptr->gameId();

    processEvents(msg_ptr->events());
}

void Client::sendEventsToGui() {
    if (!_running)
        return;

    while (_lastEventSentToGui < (int64_t)_events.back()->number()) {
        _lastEventSentToGui++;
        auto& event = _events[_lastEventSentToGui];
        char* buffer = this->buffer();
        auto len = event->toString(buffer);

        if (event->type() == PIXEL)
            len += sprintf(
                buffer + len, "%s\n",
                _players[static_cast<PixelEvent*>(event.get())->playerNumber()]
                    .c_str());
        if (event->type() == PLAYER_ELIMINATED)
            len += sprintf(
                buffer + len, "%s\n",
                _players[static_cast<PlayerEliminatedEvent*>(event.get())
                             ->playerNumber()]
                    .c_str());

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
        // debug("Received {%s} from GUI\n", line.c_str());
        if (line.compare("LEFT_KEY_DOWN") == 0)
            _turnDirection = -1;
        else if (line.compare("LEFT_KEY_UP") == 0)
            _turnDirection = 0;
        if (line.compare("RIGHT_KEY_DOWN") == 0)
            _turnDirection = 1;
        if (line.compare("RIGHT_KEY_UP") == 0)
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
        checkForGameOver();
        sendEventsToGui();
        receiveTurnDirection();
    }
}

int main(int argc, char** argv) {
    Client client(argc, argv);
    client.run();
    return 0;
}

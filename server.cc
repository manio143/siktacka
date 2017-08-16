// Main application

#include <algorithm>
#include <vector>
#include <memory>
#include "math.h"
#include "time.h"

#include "sock.h"
#include "messages.h"
#include "events.h"
#include "connectors.h"
#include "arguments.h"
#include "random.h"
#include "player.h"
#include "client.h"
#include "pixelboard.h"
#include "err.h"
#include "exception.h"

using ClientsContainer = std::vector<Client>;
using PlayersContainer = std::vector<Player>;
using EventsContainer = std::vector<std::shared_ptr<Event>>;

enum State { idle, ready, running };

class Server {
   private:
    UdpSock _sock;
    ServerArguments _arguments;
    Random _random;
    PixelBoard _pixelBoard;
    ClientsContainer _clients;
    PlayersContainer _players;
    EventsContainer _events;
    State _state;

    uint32_t _gameId;
    int _lastEventSent = -1;

    void handleIncomingPackets();
    bool addOrUpdateClient(Client& client);
    bool checkForIncomingPackets();

    void initializePlayers();

    void newGame();
    void updateGame();
    void gameOver();

    void cleanClients();
    int activePlayers();
    void sendEventsToAll();
    void sendEvents(Client& client, int nextEvent);
    bool nextRound();
    bool clientsReadyToStart();
    std::vector<std::string> playerNames();
    uint32_t nextEventNumber();

    void onNewGame();
    void onPixel(Player& player);
    void onPlayerEliminated(Player& player);
    void onGameOver();

   public:
    Server(int argc, char** argv)
        : _arguments(argc, argv),
          _state(idle),
          _sock(0),
          _random(0),
          _pixelBoard(0, 0) {
        _sock = UdpConnector::initServer(_arguments.port);
        _random = Random(_arguments.randomSeed);
        _pixelBoard = PixelBoard(_arguments.width, _arguments.height);
    }

    void run();

    ~Server() { _sock.close(); }
};

bool Server::addOrUpdateClient(Client& client) {
    bool send = true;
    bool update = false;
    for (size_t i = 0; i < _clients.size(); i++) {
        auto& lclient = _clients[i];
        if (lclient == client) {
            lclient.time = client.time;
            lclient.turnDirection = client.turnDirection;
            update = true;
        } else if (lclient.ip == client.ip) {
            if (client.sessionId > lclient.sessionId) {
                _players[lclient.playerIndex].clientIndex = -1;
                _clients[i] = client;
                update = true;
            } else {
                send = false;
                update = true;
            }
        }
    }

    if (!update)
        _clients.push_back(client);

    return send;
}

void Server::handleIncomingPackets() {
    char buffer[MAX_PACKET_SIZE];
    memset(buffer, 0, MAX_PACKET_SIZE);

    IPAddress ip;

    _sock.readFrom(buffer, MAX_PACKET_SIZE, ip);

    std::shared_ptr<ClientMessage> msg_ptr;
    try {
        ClientMessage::deserialize(buffer, &msg_ptr);
    } catch (InvalidSizeException isex) {
        debug("Invalid message received (too small).\n");
        return;
    } catch (InvalidValueException ivex) {
        debug("Invalid message received (value of %s)", ivex.what());
        return;
    }

    // debug("Received packet {%s, %d}\n", msg_ptr->playerName().c_str(),
    // msg_ptr->turnDirection());

    Client client(msg_ptr->sessionId(), msg_ptr->turnDirection(),
                  msg_ptr->playerName(), ip);

    for (auto& lclient : _clients)
        if (lclient.playerName == client.playerName && lclient.ip != client.ip)
            return;

    if (addOrUpdateClient(client))
        sendEvents(client, msg_ptr->nextExpectedEventNumber());
}

bool Server::nextRound() {
    static struct timespec prev = {0, 0};
    static struct timespec next = {0, 0};

    int nanosec = (1.0 / _arguments.roundsPerSecond) * 1000 * 1000 * 1000;

    if (clock_gettime(CLOCK_MONOTONIC, &next))
        err("clock_gettime");
    if (next.tv_sec > prev.tv_sec || next.tv_nsec - prev.tv_nsec > nanosec) {
        prev = next;
        return true;
    }
    return false;
}

bool Server::checkForIncomingPackets() {
    auto p = _sock.poll();
    if (p == Success)
        return true;
    if (p == Timeout)
        return false;
    else
        err("poll encountered an error.\n");
    return false;
}

void Server::cleanClients() {
    auto time = ::time(NULL);
    for (size_t i = 0; i < _clients.size(); i++)
        if (time - _clients[i].time > 2) {
            if (_clients[i].playerIndex > 0) {
                _players[_clients[i].playerIndex].clientIndex = -1;
            }
            _clients[i] = _clients.back();
            _clients.pop_back();
            if (_clients[i].playerIndex > 0)
                _players[_clients[i].playerIndex].clientIndex = i;
        }
}

int Server::activePlayers() {
    int active = 0;
    for (auto& player : _players)
        if (player.active)
            active++;
    return active;
}

std::vector<std::string> Server::playerNames() {
    std::vector<std::string> v;
    for (auto& player : _players)
        v.push_back(player.name);
    return v;
}

uint32_t Server::nextEventNumber() {
    if (_events.size() > 0)
        return _events.back()->number() + 1;
    else
        return 0;
}

void Server::onNewGame() {
    debug("NEW GAME\n");
    auto names = playerNames();
    auto ev_ptr = std::make_shared<NewGameEvent>(
        nextEventNumber(), _arguments.width, _arguments.height, names);
    _events.push_back(ev_ptr);
}

void Server::onPixel(Player& player) {
    debug("PIXEL {%d, %d} - %s\n", player.x(), player.y(), player.name.c_str());
    _events.push_back(std::make_shared<PixelEvent>(
        nextEventNumber(), player.number, player.x(), player.y()));
    _pixelBoard.set(player);
}

void Server::onPlayerEliminated(Player& player) {
    debug("PLAYER ELIMINATED - %s\n", player.name.c_str());
    _events.push_back(std::make_shared<PlayerEliminatedEvent>(nextEventNumber(),
                                                              player.number));
    player.active = false;
}

void Server::onGameOver() {
    debug("GAME OVER\n");
    _events.push_back(std::make_shared<GameOverEvent>(nextEventNumber()));
}

bool Server::clientsReadyToStart() {
    if (_clients.size() < 2)
        return false;

    for (auto& client : _clients) {
        if (client.playerName != "")
            if (client.turnDirection == 0)
                return false;
    }
    return true;
}

void Server::initializePlayers() {
    for (size_t i = 0; i < _clients.size(); i++) {
        if (!_clients[i].playerName.empty()) {
            float x = (_random.next() % _arguments.width) + 0.5;
            float y = (_random.next() % _arguments.height) + 0.5;
            int dir = _random.next() % 360;
            Player player(x, y, dir, _clients[i].playerName.substr());
            player.clientIndex = i;
            _players.push_back(player);
        }
    }

    std::sort(_players.begin(), _players.end());

    for (size_t i = 0; i < _players.size(); i++) {
        auto& player = _players[i];

        _clients[player.clientIndex].playerIndex = i;

        player.number = i;
        player.active = true;

        debug("Initialized player {%s}\n", player.name.c_str());
    }
}

void Server::sendEventsToAll() {
    for (auto& client : _clients)
        sendEvents(client, _lastEventSent + 1);
    if (_events.size() > 0)
        _lastEventSent = _events.back()->number();
}

void Server::sendEvents(Client& client, int nextEvent) {
    char buffer[MAX_PACKET_SIZE];

    while (nextEvent < _events.size()) {
        memset(buffer, 0, MAX_PACKET_SIZE);

        size_t size = 0;

        size_t spaceLeft = MAX_PACKET_SIZE - sizeof(uint32_t);
        EventsContainer eventsToSend;
        while (nextEvent < _events.size()) {
            if (spaceLeft < _events[nextEvent]->totalLen())
                break;
            spaceLeft -= _events[nextEvent]->totalLen();
            eventsToSend.push_back(_events[nextEvent]);
            nextEvent++;
        }

        ServerMessage sm(_gameId, eventsToSend);
        size = sm.serialize(buffer);

        _sock.writeTo(buffer, size, client.ip);
    }
}

void Server::newGame() {
    debug("Starting game\n");

    _events.clear();
    _players.clear();
		_pixelBoard.clear();

    _state = running;
    _gameId = _random.next();

    initializePlayers();

    onNewGame();

    for (auto& player : _players) {
        if (_pixelBoard.isSet(player))
            onPlayerEliminated(player);
        else
            onPixel(player);
    }
}

void Server::updateGame() {
    for (auto& player : _players) {
        if (!player.active)
            continue;

        auto turn_dir = player.clientIndex >= 0
                            ? _clients[player.clientIndex].turnDirection
                            : 0;
        if (turn_dir > 0)
            player.direction =
                (player.direction + _arguments.turningSpeed) % 360;
        else if (turn_dir < 0) {
            player.direction =
                (player.direction + 360 - _arguments.turningSpeed) % 360;
        }

        int fx = player.x(), fy = player.y();

        double angle = ((1.0 * player.direction) / 360.0) * (2.0 * M_PI);
        player.setX(player.raw_x() + cos(angle));
        player.setY(player.raw_y() + sin(angle));

        int nfx = player.x(), nfy = player.y();
        if (fx != nfx || fy != nfy) {
            if (_pixelBoard.isSet(player))
                onPlayerEliminated(player);
            else
                onPixel(player);
        }
    }
}

void Server::gameOver() {
    _state = idle;
    onGameOver();
}

void Server::run() {
    do {
        cleanClients();

        if (checkForIncomingPackets())
            handleIncomingPackets();

        if (_state == idle && clientsReadyToStart())
            _state = ready;

        if (_state == ready)
            newGame();

        if (_state == running && nextRound())
            updateGame();

        if (_state == running && activePlayers() <= 1)
            gameOver();

        sendEventsToAll();
    } while (true);
}

int main(int argc, char** argv) {
    Server server(argc, argv);
    server.run();
    return 0;
}

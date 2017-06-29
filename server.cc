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
#include "pixelboard.h"

using ClientsContainer = std::vector<Client>;
using PlayersContainer = std::vector<Player>;
using EventsContainer = std::vector<std::shared_ptr<Event>>;

enum State {
    idle,
    ready,
    running
}

class Server {
   private:
    UdpSock _sock;
    ServerArguments _arguments; //TODO
    Random _random; //TODO
    PixelBoard _pixelBoard; //TODO
    ClientsContainer _clients;
    PlayersContainer _players;
    EventsContainer _events;
    State _state;

    uint32_t _gameId;

    void handleIncomingPacket(); //TODO
    bool checkForIncomingPackets();

    void initializePlayers();

    void newGame();
    void updateGame();
    void gameOver();

    void cleanClients();
    int activePlayers();
    void sendEventsToAll(); //TODO
    bool nextRound(); //TODO (everyXms)

    void onNewGame(); //TODO
    void onPixel(Player& player); //TODO
    void onEliminatePlayer(Player& player); //TODO
    void onGameOver(); //TODO

   public:
    Server(int argc, char** argv) : _arguments(argc, argv), _state(idle) {
        _sock = UdpConnector::initServer(/*...*/);
        _random = Random(_arguments.randomSeed);
        _pixelBoard = PixelBoard(_arguments.width, _arguments.height);
    }

    void run();
}

bool Server::checkForIncomingPackets() {
    auto p = _sock.poll();
    if (p == Success)
        return true;
    if (p == Timeout)
        return false;
    else
        err("poll encountered an error.\n");
}

void Server::cleanClients() {
    for (size_t i = 0; i < _clients.size(); i++)
        if (time - _clients[i].time > 2 * 60) {
            if (_clients[i].playerIndex > 0) {
                _players[clients[i].playerIndex].clientIndex = -1;
                _players[clients[i].playerIndex].active = false;
            }
            _clients[i] = clients.back();
            _clients.pop_back();
            if (_clients[i].playerIndex > 0)
                _players[clients[i].playerIndex].clientIndex = i;
        }
}

int Server::activePlayers() {
    int active = 0;
    for(auto & player : _players)
        if(player.active)
            active++;
    return active;
}

void Server::initializePlayers() {
    for (size_t i = 0; i < clients.size(); i++) {
        if (!_clients[i].playerName.empty()) {
            Player player((_random.next() % maxx) + 0.5,
                          (_random.next() % maxy) + 0.5, _random.next() % 360,
                          clients[i].playerName.substr());
            player.clientIndex = i;
            players.push_back(player);
        }
    }

    std::sort(_players.begin(), _players.end());
    // TODO: Player must implement compare based on playerName

    for (size_t i = 0; i < _players.size(); i++) {
        auto& player = _players[i];

        _clients[player.clientIndex].playerIndex = i;

        player.number = i;
        player.active = true;

        debug("Initialized player {%s}", player.player_name);
    }
}

void Server::newGame() {
    debug("Starting game\n");

    _events.clear();
    _players.clear();

    _state = running;
    _gameId = _random.next();

    initializePlayers();

    onNewGame();

    for (auto& player : _players) {
        if (pixel_board(player))
            onEliminatePlayer(player);
        else
            onPixel(player);
    }
}

void Server::updateGame() {
    for (auto& player : _players) {
        if (!player.active)
            continue;

        auto turn_dir = clients[player.client_index].turn_direction;
        if (turn_dir > 0)
            player.direction = (player.direction + args.turning_speed) % 360;
        else if (turn_dir < 0) {
            player.direction = (player.direction + args.turning_speed) % 360;
            if (player.direction < 0)
                player.direction += 360;
        }

        int fx = floor(player.x), fy = floor(player.y);

        double angle = 1.0 * player.direction / 360.0 * 2.0 * M_PI;
        player.x += cos(angle);
        player.y += sin(angle);

        int nfx = floor(player.x), nfy = floor(player.y);
        if (fx != nfx || fy != nfy) {
            if (pixel_board(player))
                onEliminatePlayer(player);
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

        if (_state == idle && _clients.readyToStart())
            _state = ready;

        if (_state == ready)
            newGame();

        if (_state == running && nextRound())
            updateGame();

        if (_state == running && activePlayers() == 1)
            gameOver();

        sendEventsToAll();
    } while (true);
}

int main(int argc, char** argv) {
    Server server(argc, argv);
    server.run();
    return 0;
}
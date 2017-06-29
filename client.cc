// Main application

#include <vector>
#include <memory>
#include "time.h"

#include "sock.h"
#include "messages.h"
#include "events.h"
#include "connectors.h"
#include "arguments.h"

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
    std::vector<std::string> players;

    void sendRequestToServer(); //TODO
    void receiveFromServer(); //TODO
    void processEvents(); //TODO
    void receiveTurnDirection(); //TODO
    bool every20ms(); //TODO

    public:
        Client(int argc, char ** argv) : _arguments(argc, argv), _sessionId(time(NULL)) {
            _serverSock = UdpConnector::connectTo(/*...*/);
            _guiSock = TcpConnector::connectTo(/*...*/);
        }

        void run();
}

void Server::run() {
    while (true) {
        if (every20ms())
            sendRequestToServer();
        receiveFromServer();
        processEvents();
        receiveTurnDirection();
    }
}

int main(int argc, char ** argv) {
    Client client(argc, argv);
    client.run();
    return 0;
}

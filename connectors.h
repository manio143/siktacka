#pragma once

#include <string>
#include <stdint.h>

#include "sock.h"

class TcpConnector {
    public:
        static Sock connectTo(std::string host, uint16_t port);
};

class UdpConnector {
    public:
        static UdpSock connectTo(std::string host, uint16_t port);

        static UdpSock initServer(uint16_t port);
};
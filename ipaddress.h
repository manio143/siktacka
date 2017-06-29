#pragma once

#include <string>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

class IPAddress {
    public:
        IPAddress(std::string host); //TODO
        IPAddress() {}

        struct sockaddr_in6 sockaddr;
}
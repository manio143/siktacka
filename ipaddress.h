#pragma once

#include <string>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

class IPAddress {
   public:
    IPAddress() {}

    struct sockaddr_in6 sockaddr;

    bool operator==(const IPAddress& ip2) {
        return sockaddr.sin6_port == ip2.sockaddr.sin6_port &&
               memcmp(sockaddr.sin6_addr.s6_addr, ip2.sockaddr.sin6_addr.s6_addr,
                      sizeof(ip2.sockaddr.sin6_addr.s6_addr)) == 0;
    }

    bool operator!=(const IPAddress& ip2) {
        return !(*this == ip2);
    }
};
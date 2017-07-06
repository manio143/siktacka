#pragma once

#include <string>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

class IPAddress {
   public:
    IPAddress() {}

    struct sockaddr_in6 sockaddr;

    bool operator==(const IPAddress& ip1, const IPAddress& ip2) {
        return ip1.sockaddr.sin6_port == ip2.sockaddr.sin6_port &&
               memcmp(ip1.sockaddr.sin6_addr.s6_addr, ip2.sockaddr.sin6_addr.s6_addr,
                      sizeof(ip2.sockaddr.sin6_addr.s6_addr)) == 0;
    }
}
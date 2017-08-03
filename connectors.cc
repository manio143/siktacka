#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <string.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <netdb.h>

#include "connectors.h"
#include "err.h"

UdpSock UdpConnector::initServer(uint16_t port) {
    int sock = socket(PF_INET6, SOCK_DGRAM, IPPROTO_UDP);

    if (sock < 0)
        err("Couldn't create socket\n");

    struct sockaddr_in6 server_addr;
    server_addr.sin6_family = AF_INET6;
    server_addr.sin6_addr = in6addr_any;
    server_addr.sin6_port = htons(port);
    int b = bind(sock, (struct sockaddr*)&server_addr, (socklen_t)sizeof(server_addr));
    if (b < 0) {
        err("Binding stream socket (:%d)\n", port);
    }

    if (fcntl(sock, F_SETFL, O_NONBLOCK) < 0) /* tryb nieblokujący */
        err("fcntl");

    return UdpSock(sock);
}

int get_host_addrinfo(int family,
                      struct addrinfo** addr_out,
                      const char* host,
                      struct addrinfo* addr_hints,
                      bool udp) {
    (void)memset(addr_hints, 0, sizeof(struct addrinfo));
    addr_hints->ai_family = family;
    addr_hints->ai_socktype = udp ? SOCK_DGRAM : SOCK_STREAM;
    addr_hints->ai_protocol = udp ? IPPROTO_UDP : IPPROTO_TCP;

    return getaddrinfo(host, NULL, addr_hints, addr_out) != 0;
}

int get_sockaddr6(struct sockaddr_in6* sockaddr, const char* host, uint16_t port, bool udp) {
    struct addrinfo addr_hints;
    struct addrinfo* addr;

    if (get_host_addrinfo(AF_INET6, &addr, host, &addr_hints, udp))
        return -1;

    memset(sockaddr, 0, sizeof(struct sockaddr_in6));
    sockaddr->sin6_family = AF_INET6;
    memcpy(sockaddr->sin6_addr.s6_addr,
           ((struct sockaddr_in6*)(addr->ai_addr))->sin6_addr.s6_addr,
           sizeof(struct in6_addr));  // address IP
    sockaddr->sin6_port = htons(port);

    freeaddrinfo(addr);
    return 0;
}

int get_sockaddr4(struct sockaddr_in* sockaddr, const char* host, uint16_t port, bool udp) {
    struct addrinfo addr_hints;
    struct addrinfo* addr;

    if (get_host_addrinfo(AF_INET, &addr, host, &addr_hints, udp))
        return -1;

    memset(sockaddr, 0, sizeof(struct sockaddr_in));
    sockaddr->sin_family = AF_INET;
    sockaddr->sin_addr.s_addr =
        ((struct sockaddr_in*)(addr->ai_addr))->sin_addr.s_addr;  // address IP
    sockaddr->sin_port = htons(port);

    freeaddrinfo(addr);

    return 0;
}

Sock TcpConnector::connectTo(std::string host, uint16_t port) {
    struct sockaddr_in6 addrip6;
    struct sockaddr_in addrip4;
    int sock = -1;

    debug("Connecting to %s:%d over TCP\n", host.c_str(), port);

    debug("Trying with IPv6\n");
    if (!get_sockaddr6(&addrip6, host.c_str(), port, false))
        sock = socket(AF_INET6, SOCK_STREAM, 0);
    if (sock < 0 || connect(sock, (sockaddr*)&addrip6, sizeof(addrip6)) != 0) {
        // IPv4 check
        close(sock);
        debug("Retrying with IPv4\n");
        get_sockaddr4(&addrip4, host.c_str(), port, false);
        sock = socket(AF_INET, SOCK_STREAM, 0);
        if (connect(sock, (sockaddr*)&addrip4, sizeof(addrip4)) != 0)
            err("Could not connect\n");
    }

    int flag = 1;
    setsockopt(sock,         /* socket affected */
               IPPROTO_TCP,  /* set option at TCP level */
               TCP_NODELAY,  /* name of option */
               (char*)&flag, /* the cast is historical cruft */
               sizeof(int)); /* length of option value */

    return Sock(sock);
}

UdpSock UdpConnector::connectTo(std::string host, uint16_t port) {
    struct sockaddr_in6 addrip6;
    struct sockaddr_in addrip4;
    int sock = -1;

    debug("Connecting to %s:%d over UDP\n", host.c_str(), port);

    debug("Trying with IPv6\n");
    if (!get_sockaddr6(&addrip6, host.c_str(), port, true))
        sock = socket(AF_INET6, SOCK_DGRAM, 0);
    if (sock < 0 || connect(sock, (sockaddr*)&addrip6, sizeof(addrip6)) != 0) {
        // IPv4 check
        close(sock);
        debug("Retrying with IPv4\n");
        get_sockaddr4(&addrip4, host.c_str(), port, true);
        sock = socket(AF_INET, SOCK_DGRAM, 0);
        if (connect(sock, (sockaddr*)&addrip4, sizeof(addrip4)) != 0)
            err("Could not connect\n");
    }
    return UdpSock(sock);
}
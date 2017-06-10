// net.h
#include <fcntl.h>

#include "types.h"
#include "err.h"

#ifndef M_NET
#define M_NET

#ifdef CLIENT
// TODO: make also IPv4 versions
int get_host_addrinfo(int family,
                      addrinfo_t** addr_out,
                      char* host,
                      addrinfo_t* addr_hints,
                      bool udp) {
    (void)memset(addr_hints, 0, sizeof(struct addrinfo));
    addr_hints->ai_family = family;
    addr_hints->ai_socktype = udp ? SOCK_DGRAM : SOCK_STREAM;
    addr_hints->ai_protocol = udp ? IPPROTO_UDP : IPPROTO_TCP;

    return getaddrinfo(host, NULL, addr_hints, addr_out) != 0;
}

int get_sockaddr6(sockaddr_t* sockaddr, char* host, uint16_t port, bool udp) {
    addrinfo_t addr_hints;
    addrinfo_t* addr;

    if (get_host_addrinfo(AF_INET6, &addr, host, &addr_hints, udp))
        return -1;

    memset(sockaddr, 0, sizeof(sockaddr_t));
    sockaddr->sin6_family = AF_INET6;
    memcpy(sockaddr->sin6_addr.s6_addr,
           ((struct sockaddr_in6*)(addr->ai_addr))->sin6_addr.s6_addr,
           sizeof(struct in6_addr));  // address IP
    sockaddr->sin6_port = htons(port);

    freeaddrinfo(addr);
    return 0;
}

int get_sockaddr4(sockaddr4_t* sockaddr, char* host, uint16_t port, bool udp) {
    addrinfo_t addr_hints;
    addrinfo_t* addr;

    if (get_host_addrinfo(AF_INET, &addr, host, &addr_hints, udp))
        return -1;

    memset(sockaddr, 0, sizeof(sockaddr4_t));
    sockaddr->sin_family = AF_INET;
    sockaddr->sin_addr.s_addr =
        ((struct sockaddr_in*)(addr->ai_addr))->sin_addr.s_addr;  // address IP
    sockaddr->sin_port = htons(port);

    freeaddrinfo(addr);

    return 0;
}

#define TCP 1
#define UDP 2

int connect_to(char* host, int port, int sock_type) {
    sockaddr_t addrip6;
    sockaddr4_t addrip4;
    int sock = -1;

    if (!get_sockaddr6(&addrip6, host, port, sock_type == UDP))
        sock = socket(AF_INET6, sock_type == UDP ? SOCK_DGRAM : SOCK_STREAM, 0);
    if (sock < 0 || connect(sock, (sockaddr*)&addrip6, sizeof(addrip6)) != 0) {
        // IPv4 check
        close(sock);
        get_sockaddr4(&addrip4, host, port, sock_type == UDP);
        sock = socket(AF_INET, sock_type == UDP ? SOCK_DGRAM : SOCK_STREAM, 0);
        if (connect(sock, (sockaddr*)&addrip4, sizeof(addrip4)) != 0)
            return -1;
    }
    return sock;
}
#endif

void send_bytes(int sock, void* bytes, int length, sockaddr_t* sockaddr) {
    sendto(sock, bytes, length, 0, (struct sockaddr*)sockaddr,
           (socklen_t)sizeof(sockaddr_t));
}

int receive_bytes(int sock,
                  void* buffer,
                  int max_length,
                  sockaddr_t* sockaddr_out) {
    socklen_t socklen = (socklen_t)sizeof(sockaddr_t);
    return recvfrom(sock, buffer, max_length, 0, (struct sockaddr*)sockaddr_out,
                    &socklen);
}

void init_socket(pollfd_t* out_pollfd, uint16_t port) {
    struct sockaddr_in6 server;

    int sock = socket(PF_INET6, SOCK_DGRAM, IPPROTO_UDP);
    if (sock < 0)
        err("Couldn't create socket\n");

    out_pollfd->fd = sock;
    out_pollfd->events = POLLIN;

    server.sin6_family = AF_INET6;
    server.sin6_addr = in6addr_any;
    server.sin6_port = htons(port);
    if (bind(sock, (struct sockaddr*)&server, (socklen_t)sizeof(server)) < 0) {
        err("Binding stream socket (:%d)\n", port);
    }

    if (fcntl(sock, F_SETFL, O_NONBLOCK) < 0) /* tryb nieblokujący */
        err("fcntl");
}

#endif
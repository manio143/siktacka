// net.h
#include <fcntl.h>

#include "types.h"
#include "err.h"

#ifndef M_NET
#define M_NET

#ifdef CLIENT
//TODO: make also IPv4 versions
void get_host_addrinfo(addrinfo_t** addr_out,
                       char* host,
                       addrinfo_t* addr_hints, bool udp) {
    (void)memset(addr_hints, 0, sizeof(struct addrinfo));
    addr_hints->ai_family = AF_INET6;
    addr_hints->ai_socktype = udp ? SOCK_DGRAM : SOCK_STREAM;
    addr_hints->ai_protocol = udp ? IPPROTO_UDP : IPPROTO_TCP;

    if (getaddrinfo(host, NULL, addr_hints, addr_out) != 0) {
        err("getaddrinfo (%s)", host);
    }
}

void get_sockaddr(sockaddr_t* sockaddr, char * host, uint16_t port, bool udp) {
    addrinfo_t addr_hints;
    addrinfo_t* addr;

    get_host_addrinfo(&addr, host, &addr_hints, udp);

    memset(sockaddr, 0, sizeof(sockaddr_t));
    sockaddr->sin6_family = AF_INET6;
    memcpy(sockaddr->sin6_addr.s6_addr,
        ((struct sockaddr_in6*)(addr->ai_addr))->sin6_addr.s6_addr, sizeof(struct in6_addr));  // address IP
    sockaddr->sin6_port = htons(port);

    freeaddrinfo(addr);
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
    return recvfrom(sock, buffer, max_length, 0, (struct sockaddr*)sockaddr_out, &socklen);
}

void init_socket(pollfd_t * out_pollfd, uint16_t port) {
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
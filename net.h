// net.h
#include <fcntl.h>

#include "types.h"
#include "err.h"

#ifndef M_NET
#define M_NET

void get_host_addrinfo(addrinfo_t** addr_out,
                       arguments_t* args,
                       addrinfo_t* addr_hints) {
    (void)memset(addr_hints, 0, sizeof(struct addrinfo));
    addr_hints->ai_family = AF_INET;  // IPv4
    addr_hints->ai_socktype = SOCK_DGRAM;
    addr_hints->ai_protocol = IPPROTO_UDP;
    addr_hints->ai_flags = 0;
    addr_hints->ai_addrlen = 0;
    addr_hints->ai_addr = NULL;
    addr_hints->ai_canonname = NULL;
    addr_hints->ai_next = NULL;
    if (getaddrinfo(args->host, NULL, addr_hints, addr_out) != 0) {
        err("getaddrinfo");
    }
}

void get_sockaddr(sockaddr_t* sockaddr, arguments_t* args) {
    addrinfo_t addr_hints;
    addrinfo_t* addr;

    get_host_addrinfo(&addr, args, &addr_hints);

    sockaddr->sin_family = AF_INET;  // IPv4
    sockaddr->sin_addr.s_addr =
        ((sockaddr_t*)(addr->ai_addr))->sin_addr.s_addr;  // address IP
    sockaddr->sin_port = htons(args->port);

    freeaddrinfo(addr);
}

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

//TODO: to IPv6
int init_socket(pollfd_t * out_pollfd, uint16_t port) {
    sockaddr_t server;

    int sock = socket(PF_INET, SOCK_DGRAM, 0);
    if (sock < 0)
        err("Couldn't create socket\n");

    out_pollfd.fd = sock;
    out_pollfd.events = POLLIN;

    server.sin_family = AF_INET;
    server.sin_addr.s_addr = htonl(INADDR_ANY);
    server.sin_port = htons(port);
    if (bind(sock, (struct sockaddr*)&server, (socklen_t)sizeof(server)) < 0) {
        err("Binding stream socket");
    }

    if (fcntl(sock, F_SETFL, O_NONBLOCK) < 0) /* tryb nieblokujący */
        err("fcntl");
}

#endif
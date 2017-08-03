#pragma once

#include <sys/socket.h>
#include <unistd.h>

#include "ipaddress.h"

enum PollResult { Success, Timeout, Error };

class Sock {
   protected:
    int _sock;

   public:
    Sock(int sock) : _sock(sock) {}

    PollResult poll();
    int read(void* buffer, size_t size);
    int write(void* buffer, size_t size);

    void close() { ::close(_sock); }
};

class UdpSock : public Sock {
   public:
    UdpSock(int sock) : Sock(sock) {}

    int readFrom(void* buffer, size_t size, IPAddress& address);
    int writeTo(void* buffer, size_t size, IPAddress& address);
};
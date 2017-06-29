#pragma once

#include <sys/socket.h>

#include "ipaddress.h"

enum PollResult { Success, Timeout, Error };

class Sock {
   private:
    int _sock;

   public:
    Sock(int sock) : _sock(sock) {}

    PollResult poll();
    int read(void* buffer, size_t size);
    int write(void* buffer, size_t size);

    ~Sock() { close(_sock); }
}

class UdpSock : public Sock {
   public:
    int readFrom(void* buffer, size_t size, IPAddress& address);
    int writeTo(void* buffer, size_t size, IPAddress& address);
}
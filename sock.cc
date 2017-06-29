#include <sys/socket.h>
#include <poll.h>
#include "sock.h"

PollResult Sock::poll() {
    struct pollfd pollfd;
    pollfd.fd = this->_sock;
    pollfd.events = POLLIN;

    int r = ::poll(&pollfd, 1, 1);

    if (r < 0)
        return Error;
    else if (r == 0)
        return Timeout;
    else if (pollfd.revents & POLLERR || pollfd.revents & POLLHUP ||
             pollfd.revents & POLLNVAL)
        return Error;
    else
        return Success;
}

int Sock::read(void* buffer, size_t size) {
    return ::read(this->_sock, buffer, size);
}

int Sock::write(void* buffer, size_t size) {
    return ::write(this->_sock, buffer, size);
}

int UdpSock::readFrom(void* buffer, size_t size, IPAddress& address) {
    socklen_t socklen = (socklen_t)sizeof(struct sockaddr_in6);
    return recvfrom(this->_sock, buffer, size, 0,
                    (struct sockaddr*)&address.sockaddr, &socklen);
}

int UdpSock::writeTo(void* buffer, size_t size, IPAddress& address) {
    return sendto(this->_sock, buffer, size, 0,
                  (struct sockaddr*)&address.sockaddr,
                  (socklen_t)sizeof(struct sockaddr_in6));
}

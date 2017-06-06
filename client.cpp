#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <stdarg.h>
#include <signal.h>

#define TRUE 1
#define FALSE 0
#define BUFFER_SIZE 4000

#define htonll(x) ((1==htonl(1)) ? (x) : ((uint64_t)htonl((x) & 0xFFFFFFFF) << 32) | htonl((x) >> 32))
#define ntohll(x) ((1==ntohl(1)) ? (x) : ((uint64_t)ntohl((x) & 0xFFFFFFFF) << 32) | ntohl((x) >> 32))


void err(const char* fmt, ...);

typedef struct arguments {
    uint64_t timestamp;
    char c;
    char* host;
    uint16_t port;
} arguments_t;

struct send_packet {
    uint64_t network_timestamp;
    char c;
} __attribute__((packed));
typedef struct send_packet send_packet_t;

typedef struct addrinfo addrinfo_t;
typedef struct sockaddr_in sockaddr_t;

void fill_args(arguments_t* args_out, int argc, char** argv);

void get_host_addrinfo(addrinfo_t** addr_out,
                       arguments_t* args,
                       addrinfo_t* addr_hints);

void get_sockaddr(sockaddr_t* sockaddr, arguments_t* args);

void make_packet(send_packet_t* packet_out, arguments_t* args);

void send_bytes(int sock, void* bytes, int length, sockaddr_t* sockaddr);

int receive_bytes(int sock,
                  void* buffer,
                  int max_length,
                  sockaddr_t* sockaddr_out);

static int finish = FALSE;

/* Obsługa sygnału kończenia */
static void catch_int(int sig) {
    finish = TRUE;
}

char buffer[BUFFER_SIZE];

int main(int argc, char** argv) {
    int sock;
    arguments_t args;
    sockaddr_t sockaddr;
    sockaddr_t incoming_sockaddr;
    send_packet_t packet;

    // if (signal(SIGINT, catch_int) == SIG_ERR) {
    //     err("Unable to change signal handler\n");
    // }

    fill_args(&args, argc, argv);
    get_sockaddr(&sockaddr, &args);

    sock = socket(PF_INET, SOCK_DGRAM, 0);
    if (sock < 0)
        err("Couldn't create socket\n");

    make_packet(&packet, &args);

    send_bytes(sock, &packet, sizeof(packet), &sockaddr);

    while (1) {
        if (finish == TRUE)
            break;

        memset(&buffer, 0, sizeof(buffer));
        int received = receive_bytes(sock, &buffer, sizeof(buffer) - 1,
                                     &incoming_sockaddr);
        if (received < 0) {
            err("Error reading data from socket\n");
        }
        send_packet_t * pack = (send_packet_t *)buffer;
        printf("%u %c %s\n", ntohl(pack->network_timestamp), pack->c, buffer + sizeof(send_packet_t));
    }

    if (close(sock) == -1)
        err("close");

    return 0;
}

void fill_args(arguments_t* args_out, int argc, char** argv) {
    if (argc < 4)
        err("Too few arguments\n");

    args_out->timestamp = atol(argv[1]);
    args_out->c = argv[2][0];
    args_out->host = argv[3];
    args_out->port = argc >= 5 ? atoi(argv[4]) : 20160;
}


void make_packet(send_packet_t* packet_out, arguments_t* args) {
    packet_out->network_timestamp = htonll(args->timestamp);
    packet_out->c = args->c;
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
    return recvfrom(sock, buffer, max_length, 0, (struct sockaddr*)sockaddr_out,
                    &socklen);
}

void err(const char* fmt, ...) {
    va_list fmt_args;

    fprintf(stderr, "ERROR: ");
    va_start(fmt_args, fmt);
    vfprintf(stderr, fmt, fmt_args);
    va_end(fmt_args);
    // fprintf(stderr, " (%d; %s)\n", errno, strerror(errno));
    exit(EXIT_FAILURE);
}
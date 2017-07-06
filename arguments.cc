#include <getopt.h>
#include <time.h>
#include <string>
#include <cstdarg>
#include <string.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "arguments.h"
#include "err.h"

void ServerArguments::setDefaults() {
    width = 800;
    height = 600;
    port = 12345;
    roundsPerSecond = 50;
    turningSpeed = 6;
    randomSeed = time(NULL);
}

ServerArguments::ServerArguments(int argc, char** argv) {
    setDefaults();

    int opt;
    char o;
    while ((opt = getopt(argc, argv, "W:H:p:s:t:r:")) != -1) {
        switch (opt) {
            case 'W':
                this->width = atoi(optarg);
                break;
            case 'H':
                this->height = atoi(optarg);
                break;
            case 'p':
                this->port = atoi(optarg);
                break;
            case 's':
                this->roundsPerSecond = atoi(optarg);
                break;
            case 't':
                this->turningSpeed = atoi(optarg);
                break;
            case 'r':
                this->randomSeed = atol(optarg);
                break;
            case '?':
                o = (char)optopt;
                if (strstr("WHpstr", &o))
                    err("Option -%c requires an argument.\n", optopt);
                else if (isprint(optopt))
                    err("Unknown option `-%c'.\n", optopt);
                else
                    err("Unknown option character `\\x%x'.\n", optopt);
                break;
            default:
                err("Usage: %s [-W n] [-H n] [-p n] [-s n] [-t n] [-r n]\n",
                    argv[0]);
        }
    }
}

void break_string_on_delim(char* str, char delim, char** out_second_str) {
    while (*str) {
        if (*str == delim) {
            *str = 0;
            *out_second_str = str + 1;
            break;
        }
        str++;
    }
}

bool is_ipv6_address(const char* str) {
    struct sockaddr_in6 sa;
    return inet_pton(AF_INET6, str, &(sa.sin6_addr)) != 0;
}

ClientArguments::ClientArguments(int argc, char** argv) {
    if (argc < 3 || argc > 4)
        err("USAGE: %s player_name game_server_host[:port] "
            "[ui_server_host[:port]]\n",
            argv[0]);

    auto& name = argv[1];
    if (strlen(name) > 65)
        err("Player name too long.\n");
    for (int i = 0; true; i++) {
        if (name[i] == '\0')
            break;
        if (name[i] < 33 || name[i] > 126)
            err("Invalid character in player_name.\n");
    }
    this->playerName = std::string(name);

    char* port = NULL;
    if (!is_ipv6_address(argv[2]))
        break_string_on_delim(argv[2], ':', &port);
    this->host = argv[2];
    this->port = port == NULL ? 12345 : atoi(port);
    if (argc == 4) {
        port = NULL;
        if (!is_ipv6_address(argv[3]))
            break_string_on_delim(argv[3], ':', &port);
        this->guihost = argv[3];
        this->guiport = port == NULL ? 12346 : atoi(port);
    } else {
        this->guihost = "localhost";
        this->guiport = 12346;
    }
}
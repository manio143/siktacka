#include <vector>

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
#include <time.h>

#include <arpa/inet.h>

#include "const.h"
#include "types.h"
#include "crc32.h"
#include "err.h"
#include "hton.h"

#define CLIENT

typedef struct arguments {
    char player_name[65];
    char* host;
    uint16_t port;
    char* guihost;
    uint16_t guiport;
} arguments_t;

#include "net.h"

using namespace std;

bool is_ipv6_address(const char* str);
void fill_args(arguments_t* args_out, int argc, char** argv);

void receive_from_server(int sock);
void send_request_to_server(int sock, arguments_t& args);
void process_events(int sock);
void receive_turn_direction(int sock);

bool every_20ms();

char buffer[MAX_PACKET_SIZE];

vector<msg_event_t> events;
char* players;

uint8_t turn_direction = 0;
int last_sent_to_gui = -1;
uint64_t session_id;
bool running = false;
int current_game_id;

int main(int argc, char** argv) {
    int server_sock, gui_sock;
    arguments_t args;

    session_id = time(NULL);

    fill_args(&args, argc, argv);

    server_sock = connect_to(args.host, args.port, UDP);
    if (server_sock < 0)
        err("Couldn't connect to server\n");
    printf("Connected to server\n");

    gui_sock = connect_to(args.guihost, args.guiport, TCP);
    if (gui_sock < 0)
        err("Couldn't connect to gui\n");
    printf("Connected to gui\n");

    while (true) {
        if (every_20ms())
            send_request_to_server(server_sock, args);
        receive_from_server(server_sock);
        process_events(gui_sock);
        receive_turn_direction(gui_sock);
    }

    if (close(server_sock) == -1)
        err("close");

    return 0;
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

void substitute(char* str, size_t max_length, char from, char to) {
    for (size_t i = 0; i < max_length; i++)
        if (*(str + i) == from)
            *(str + i) = to;
}

void fill_args(arguments_t* args_out, int argc, char** argv) {
    if (argc < 3 || argc > 4)
        err("USAGE: %s player_name game_server_host[:port] "
            "[ui_server_host[:port]]\n",
            argv[0]);

    strncpy(args_out->player_name, argv[1], 64);
    for (size_t i = 0; i < sizeof(args_out->player_name); i++) {
        if (args_out->player_name[i] == '\0')
            break;
        if (args_out->player_name[i] < 33 || args_out->player_name[i] > 126)
            err("Invalid character in player_name.\n");
    }
    char* port = NULL;
    if (!is_ipv6_address(argv[2]))
        break_string_on_delim(argv[2], ':', &port);
    args_out->host = argv[2];
    args_out->port = port == NULL ? 12345 : atoi(port);
    if (argc == 4) {
        port = NULL;
        if (!is_ipv6_address(argv[3]))
            break_string_on_delim(argv[3], ':', &port);
        args_out->guihost = argv[3];
        args_out->guiport = port == NULL ? 12346 : atoi(port);
    } else {
        args_out->guihost = "localhost";
        args_out->guiport = 12346;
    }
}

void clear_buffer() {
    memset(&buffer, 0, sizeof(buffer));
}

bool is_ipv6_address(const char* str) {
    struct sockaddr_in6 sa;
    return inet_pton(AF_INET6, str, &(sa.sin6_addr)) != 0;
}

int parse_event(char* buff) {
    msg_event_t event;
    event.header = *((msg_event_header_t*)buff);
    buff += sizeof(msg_event_header_t);

    switch (event.header.event_type) {
        case NEW_GAME:
            event.event_data.new_game.player_names_size =
                ntohl(event.header.len) - sizeof(msg_event_header_t) -
                3 * sizeof(uint32_t);
            event.event_data.new_game.maxx = *((uint32_t*)buff);
            buff += sizeof(uint32_t);
            event.event_data.new_game.maxy = *((uint32_t*)buff);
            buff += sizeof(uint32_t);
            memcpy(buff, event.event_data.new_game.player_names,
                   event.event_data.new_game.player_names_size);
            substitute(event.event_data.new_game.player_names,
                       event.event_data.new_game.player_names_size - 1, 0, ' ');
            buff += event.event_data.new_game.player_names_size;
            break;
        case PIXEL:
            event.event_data.pixel = *((msg_event_data_pixel_t*)buff);
            buff += sizeof(msg_event_data_pixel_t);
            break;
        case PLAYER_ELIMINATED:
            event.event_data.player_eliminated =
                *((msg_event_data_player_eliminated_t*)buffer);
            buff += sizeof(msg_event_data_player_eliminated_t);
            break;
    }
    event.crc32 = ntohl(*((uint32_t*)buff));
    buff += sizeof(uint32_t);

    event.header = msg_event_header_ntoh(event.header);
    switch (event.header.event_type) {
        case NEW_GAME:
            event.event_data.new_game.maxx =
                ntohl(event.event_data.new_game.maxx);
            event.event_data.new_game.maxy =
                ntohl(event.event_data.new_game.maxy);
            break;
        case PIXEL:
            event.event_data.pixel.x = ntohl(event.event_data.pixel.x);
            event.event_data.pixel.y = ntohl(event.event_data.pixel.y);
            break;
    }

    // TODO: sanity check - "czy wartości mają sens?"

    if (event.crc32 == crc32((char*)&event, event.header.len)) {
        if (events.back().header.event_no == event.header.event_no - 1) {
            if (event.header.event_type <= 3)
                events.push_back(event);
            return event.header.len;
        } else {
            return MAX_PACKET_SIZE;
        }
    } else {
        return MAX_PACKET_SIZE;
    }
}

int check_sock(int sock, pollfd_t& pollfd) {
    pollfd.fd = sock;
    pollfd.events = POLLIN;
    return poll(&pollfd, 1, 1);
}

void receive_from_server(int sock) {
    pollfd_t pollfd;

    clear_buffer();

    int r = check_sock(sock, pollfd);
    if (r < 0)
        err("poll");
    if (r == 0)
        return;

    if(pollfd.revents & POLLIN)
        printf("Datagram arrived\n");
    if (pollfd.revents & POLLNVAL)
        err("Invalid socket passed to poll\n");
    else if (pollfd.revents & POLLHUP)
        err("Poll hangup error\n");
    else if (pollfd.revents & POLLERR) {
        err("Destination unreachable\n");
    }

    int received = recv(sock, &buffer, sizeof(buffer) - 1, 0);
    if (received < 0) {
        err("Error reading data from socket (%d)\n", received);
    }

    char* buff = buffer;
    int game_id = ntohl(*((uint32_t*)buff));
    buff += sizeof(uint32_t);

    if (game_id != current_game_id && running)
        return;
    current_game_id = game_id;

    while (buff < buffer + 512 &&
           ntohl(*((uint32_t*)buff)) != 0)  // while len != 0
        buff += parse_event(buff);
}

void send_request_to_server(int sock, arguments_t& args) {
    msg_from_client_t msg;
    msg.session_id = session_id;
    msg.turn_direction = turn_direction;
    if (events.size() > 0)
        msg.next_expected_event_no = events.back().header.event_no;
    else
        msg.next_expected_event_no = 0;
    strncpy(msg.player_name, args.player_name, sizeof(msg.player_name));
    // printf("Sending request to server\n");
    send(sock, &msg, sizeof_msg_from_client(&msg), 0);
}

char* get_player_name(char* player_name, int i) {
    char* ptr = players;
    for (int k = 0; k < i; k++) {
        while (*ptr != ' ' && *ptr != '\0')
            ptr++;
        ptr++;
    }
    char* begin = ptr;
    while (*ptr != ' ' && *ptr != '\0')
        ptr++;
    strncpy(player_name, begin, ptr - begin);
    return player_name;
}

void process_events(int sock) {
    if (last_sent_to_gui <
        (events.size() > 0 ? events.back().header.event_no : 0)) {
        for (auto& event : events) {
            if (event.header.event_no <= last_sent_to_gui)
                continue;
            clear_buffer();
            size_t len;
            char player_name[65];
            switch (event.header.event_type) {
                case NEW_GAME:
                    len = sprintf(buffer, "NEW_GAME %d %d %s\n",
                                  event.event_data.new_game.maxx,
                                  event.event_data.new_game.maxy,
                                  event.event_data.new_game.player_names);
                    players = event.event_data.new_game.player_names;
                    running = true;
                    break;
                case PIXEL:
                    len = sprintf(
                        buffer, "PIXEL %d %d %s\n", event.event_data.pixel.x,
                        event.event_data.pixel.y,
                        get_player_name(player_name,
                                        event.event_data.pixel.player_number));
                    break;
                case PLAYER_ELIMINATED:
                    len = sprintf(
                        buffer, "PLAYER_ELIMINATED %s\n",
                        get_player_name(
                            player_name,
                            event.event_data.player_eliminated.player_number));
                    break;
                case GAME_OVER:
                    len = sprintf(buffer, "GAME_OVER\n");
                    events.clear();
                    running = false;
                    break;
            }
            printf("Sending {%s} to gui\n", buffer);
            write(sock, buffer, len);
            last_sent_to_gui++;
        }
    }
}

void receive_turn_direction(int sock) {
    pollfd_t pollfd;
    clear_buffer();

    int r = check_sock(sock, pollfd);
    if (r < 0)
        err("poll");
    if (r == 0)
        return;


    if (pollfd.revents & POLLNVAL)
        err("Invalid socket passed to poll\n");
    else if (pollfd.revents & POLLHUP)
        err("Poll hangup error\n");
    else if (pollfd.revents & POLLERR)
        err("Poll error\n");

    if(read(sock, buffer, MAX_PACKET_SIZE - 1) <= 0)
        err("Cannot read from gui\n");
    substitute(buffer, 20, '\n', '\0');
    printf("Received {%s} from gui\n", buffer);
    if (!strcmp(buffer, "LEFT_KEY_DOWN"))
        turn_direction = -1;
    else if (!strcmp(buffer, "LEFT_KEY_UP"))
        turn_direction = 0;
    else if (!strcmp(buffer, "RIGHT_KEY_DOWN"))
        turn_direction = 1;
    else if (!strcmp(buffer, "RIGHT_KEY_UP"))
        turn_direction = 0;
}

bool every_20ms() {
    static struct timespec prev = {0, 0};
    static struct timespec next = {0, 0};

    if (clock_gettime(CLOCK_MONOTONIC, &next))
        err("clock_gettime");
    if (next.tv_sec > prev.tv_sec ||
        next.tv_nsec - prev.tv_nsec > 20000000) {  // 20ms
        prev = next;
        return true;
    }
    return false;
}
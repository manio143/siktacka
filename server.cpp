#include <vector>
#include <tuple>
#include <algorithm>

#include <math.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <stdarg.h>
#include <poll.h>
#include <signal.h>
#include <fcntl.h>
#include <time.h>
#include <limits.h>

#include "const.h"
#include "types.h"
#include "net.h"
#include "err.h"
#include "hton.h"
#include "crc32.h"

using namespace std;

typedef struct arguments {
    uint32_t width;              // 800
    uint32_t height;             // 600
    uint16_t port;               // 12345
    uint32_t rounds_per_second;  // 50
    uint32_t turning_speed;      // 6
    uint64_t random_seed;        // time(NULL)
} arguments_t;

void fill_args(arguments_t* args_out, int argc, char** argv);

tuple<bool, uint32_t> receive_msg_from_client(pollfd_t& pollfd,
                                              client_t& client);

uint64_t _random;

void random_init(arguments_t* args) {
    _random = args->random_seed;
}
uint64_t random_next() {
    uint64_t r = _random;
    _random = (_random * 279470273) % 4294967291;
    return r;
}

vector<player_state_t> players;

int player_name_cmp(player_state_t& p1, player_state_t& p2) {
    return strcmp(p1.player_name, p2.player_name);
}

int client_exists(client_t& client);
bool add_client(client_t client);
vector<client_t> clients;

bool every_Nms(int n);

int active_players = 0;

void clean_clients(time_t time) {
    for (size_t i = 1; i < clients.size(); i++)
        if (time - clients[i].time < 2 * 60) {
            if (clients[i].player_index > 0) {
                players[clients[i].player_index].client_index = -1;
                players[clients[i].player_index].active = false;
                active_players--;
            }
            clients[i] = clients.back();
            clients.pop_back();
            if (clients[i].player_index > 0)
                players[clients[i].player_index].client_index = i;
        }
}

uint32_t game_id = 0;

vector<msg_event_t> events;
int prepared = -1;
int last_event_sent = -1;

msg_event_t make_new_game_event();
msg_event_t add_pixel(player_state_t& player);
msg_event_t eliminate_player(player_state_t& player);
msg_event_t game_over_event();

void send_events(int next_event_no, client_t& client);

int* pixels;

int& get_pixel(int x, int y) {
    return pixels[x * maxy + y];
}
int& get_pixel(player_state_t& player) {
    return get_pixel(floor(player.x), floor(player.y));
}

int sock;
int main(int argc, char** argv) {
    arguments_t args;
    pollfd_t pollfd;

    fill_args(&args, argc, argv);

    random_init(&args);

    init_socket(&pollfd, args.port);
    sock = pollfd.fd;

    printf("Initialized socket\n");

    pixels = new int[maxx * maxy];

    bool running = false;

    do {
        pollfd.revents = 0;
        clean_clients(time(NULL));

        int ret = poll(&pollfd, 1, 1);
        if (ret < 0)
            err("poll");
        else if (ret > 0) {
            if (pollfd.revents & POLLIN) {
                client_t client;
                auto tup = receive_msg_from_client(pollfd, client);
                auto next_event_no = get<1>(tup);
                if (get<0>(tup))
                    send_events(next_event_no, client);
            }
        }

        bool start = clients.size() >= 2;
        for (auto& client : clients) {
            //printf("dir: %d ", client.turn_direction);
            start = start && (client.turn_direction != 0);
        }

        if (!running && start) {
            // GAME INIT
            printf("Starting game\n");
            running = true;
            game_id = random_next();
            for (size_t i = 0; i < clients.size(); i++) {
                if (strcmp("", clients[i].player_name)) {
                    player_state_t player;
                    player.x = (random_next() % maxx) + 0.5;
                    player.y = (random_next() % maxy) + 0.5;
                    player.direction = random_next() % 360;
                    strcpy(player.player_name, clients[i].player_name);
                    player.client_index = i;
                    players.push_back(player);
                }
            }
            sort(players.begin(), players.end(), player_name_cmp);

            active_players = players.size();

            events.push_back(make_new_game_event());

            for (size_t i = 0; i < players.size(); i++) {
                auto player = players[i];

                printf("Initialized player {%s}", player.player_name);

                clients[player.client_index].player_index = i;

                player.player_no = i;
                player.active = true;

                if (get_pixel(player))
                    events.push_back(eliminate_player(player));
                else
                    events.push_back(add_pixel(player));
            }
        }
        if (running && every_Nms(1 / args.rounds_per_second * 1000)) {
            // GAME_UPDATE
            for (size_t i = 0; i < players.size(); i++) {
                if(!players[i].active) continue;
                auto turn_dir = clients[players[i].client_index].turn_direction;
                if (turn_dir > 0)
                    players[i].direction =
                        (players[i].direction + args.turning_speed) % 360;
                else if (turn_dir < 0) {
                    players[i].direction =
                        (players[i].direction + args.turning_speed) % 360;
                    if (players[i].direction < 0)
                        players[i].direction += 360;
                }

                int fx = floor(players[i].x), fy = floor(players[i].y);

                double angle = players[i].direction/360 * 2 * M_PI;
                players[i].x += cos(angle);
                players[i].y += sin(angle);

                int nfx = floor(players[i].x), nfy = floor(players[i].y);
                if (fx == nfx && fy == nfy)
                    continue;
                else {
                    if (get_pixel(players[i]))
                        events.push_back(eliminate_player(players[i]));
                    else
                        events.push_back(add_pixel(players[i]));
                }
            }
        }

        if (active_players == 1) {
            running = false;
            events.push_back(game_over_event());
        }

        if ((int)events.size() > last_event_sent + 1) {
            for (auto& client : clients)
                send_events(last_event_sent + 1, client);
        }
    } while (true);

    if (close(sock) == -1)
        err("close");

    delete pixels;

    return 0;
}

void fill_args(arguments_t* args_out, int argc, char** argv) {
    args_out->width = 800;
    args_out->height = 600;
    args_out->port = 12345;
    args_out->rounds_per_second = 50;
    args_out->turning_speed = 6;
    args_out->random_seed = time(NULL);

    int opt;
    while ((opt = getopt(argc, argv, "WHpstr:")) != -1) {
        switch (opt) {
            case 'W':
                args_out->width = atoi(optarg);
                break;
            case 'H':
                args_out->height = atoi(optarg);
                break;
            case 'p':
                args_out->port = atoi(optarg);
                break;
            case 's':
                args_out->rounds_per_second = atoi(optarg);
                break;
            case 't':
                args_out->turning_speed = atoi(optarg);
                break;
            case 'r':
                args_out->random_seed = atol(optarg);
                break;
            default:
                err("Usage: %s [-W n] [-H n] [-p n] [-s n] [-t n] [-r n]\n",
                    argv[0]);
        }
    }
}

int sock_equal(sockaddr_t & s1, sockaddr_t & s2) {
    return s1.sin6_port == s2.sin6_port && memcmp(s1.sin6_addr.s6_addr, s2.sin6_addr.s6_addr, sizeof(s2.sin6_addr.s6_addr)) == 0;
}

int clients_sock_equal(client_t c1, client_t c2) {
    return sock_equal(c1.sockaddr, c2.sockaddr);
}
int clients_equal(client_t c1, client_t c2) {
    return clients_sock_equal(c1, c2) && c1.session_id == c2.session_id;
}

int client_exists(client_t client) {
    for (size_t i = 1; i < clients.size(); i++)
        if (clients_sock_equal(clients[i], client))
            return true;
    return false;
}

bool add_client(client_t client) {
    for (size_t i = 1; i < clients.size(); i++) {
        if (clients_equal(clients[i], client)) {
            clients[i].time = client.time;
            return true;
        } else if (clients_sock_equal(clients[i], client)) {
            if (client.session_id > clients[i].session_id) {
                players[clients[i].player_index].active = false;
                active_players--;
                clients[i] = client;
                return true;
            } else
                return false;
        }
    }
    clients.push_back(client);
    return true;
}
tuple<bool, uint32_t> receive_msg_from_client(pollfd_t& pollfd,
                                              client_t& client) {
    client.time = time(NULL);
    msg_from_client_t lpacket;
    memset(lpacket.player_name, 0, sizeof(lpacket.player_name));
    receive_bytes(pollfd.fd, &lpacket, sizeof(msg_from_client_t),
                  &(client.sockaddr));
    lpacket = msg_from_client_ntoh(lpacket);
    client.session_id = lpacket.session_id;
    client.turn_direction = lpacket.turn_direction;
    client.player_index = -1;
    strcpy(client.player_name, lpacket.player_name);
    bool good = true;
    for (auto& lclient : clients)
        if (!strcmp(lclient.player_name, lpacket.player_name) && !sock_equal(lclient.sockaddr, client.sockaddr))
            return make_tuple(false, 0);
    good = good && add_client(client);
    auto retval = lpacket.next_expected_event_no;
    printf("Received message {->%d, %s, nxt %d}\n", client.turn_direction, client.player_name, lpacket.next_expected_event_no);
    return make_tuple(good, retval);
}

unsigned int lenstrcpy(char dest[], const char source[]) {
    unsigned int i = 0;
    while ((dest[i] = source[i]) != '\0') {
        i++;
    }
    return i;
}

int curr_event_no = 0;

msg_event_t make_new_game_event() {
    printf("EVENT: NEW_GAME\n");
    msg_event_t event;
    event.header.event_no = curr_event_no++;
    event.header.event_type = NEW_GAME;
    msg_event_data_new_game_t ng = {.maxx = maxx, .maxy = maxy};
    event.event_data.new_game = ng;
    int player_string_len = 0;
    for (auto& player : players) {
        player_string_len += strlen(player.player_name) + 1;
        if (player_string_len > MAX_PACKET_SIZE) {
            player_string_len -= strlen(player.player_name) + 1;
            break;
        }
    }

    int i = 0;
    for (int pos = 0; pos < player_string_len; i++) {
        pos += lenstrcpy(event.event_data.new_game.player_names + pos,
                         players[i].player_name);
    }

    event.event_data.new_game.player_names_size = player_string_len;

    return event;
}

msg_event_t eliminate_player(player_state_t& player) {
    printf("EVENT: PLAYER_ELIMINATED (%d)\n", player.player_no);
    msg_event_t event;
    event.header.event_no = curr_event_no++;
    event.header.event_type = PLAYER_ELIMINATED;
    event.event_data.player_eliminated.player_number = player.player_no;
    player.active = false;
    active_players--;
    return event;
}

msg_event_t add_pixel(player_state_t& player) {
    printf("EVENT: PIXEL ([%d, %d] - %d)\n", floor(player.x), floor(player.y), player.player_no);
    msg_event_t event;
    event.header.event_no = curr_event_no++;
    event.header.event_type = PIXEL;
    event.event_data.pixel.player_number = player.player_no;
    event.event_data.pixel.x = floor(player.x);
    event.event_data.pixel.y = floor(player.y);
    get_pixel(player) = player.player_no;
    return event;
}

msg_event_t game_over_event() {
    printf("EVENT: GAME_OVER\n");
    msg_event_t event;
    event.header.event_no = curr_event_no++;
    event.header.event_type = GAME_OVER;
    return event;
}

void prepare_event(msg_event_t& event) {
    event.header.len = sizeof(msg_event_header_t);
    switch (event.header.event_type) {
        case NEW_GAME:
            event.header.len += 2 * sizeof(uint32_t) +
                                event.event_data.new_game.player_names_size;
            break;
        case PIXEL:
            event.header.len += sizeof(msg_event_data_pixel_t);
            break;
        case PLAYER_ELIMINATED:
            event.header.len += sizeof(msg_event_data_player_eliminated_t);
            break;
    }
    // CRC
    event.header.len += sizeof(uint32_t);
    event.crc32 = htonl(crc32((char*)&event, event.header.len));

    event.header = msg_event_header_hton(event.header);
    switch (event.header.event_type) {
        case NEW_GAME:
            event.event_data.new_game.maxx = htonl(event.event_data.new_game.maxx);
            event.event_data.new_game.maxy = htonl(event.event_data.new_game.maxy);
            break;
        case PIXEL:
            event.event_data.pixel.x = htonl(event.event_data.pixel.x);
            event.event_data.pixel.y = htonl(event.event_data.pixel.y);
            break;
    }
}

char send_buffer[MAX_PACKET_SIZE];

int write_to_buffer(char* buffer, int idx) {
    auto& event = events[idx];
    *((msg_event_header_t*)buffer) = event.header;
    buffer += sizeof(msg_event_header_t);

    switch (event.header.event_type) {
        case NEW_GAME:
            *((uint32_t*)buffer) = event.event_data.new_game.maxx;
            buffer += sizeof(uint32_t);
            *((uint32_t*)buffer) = event.event_data.new_game.maxy;
            buffer += sizeof(uint32_t);
            memcpy(buffer, event.event_data.new_game.player_names,
                   event.event_data.new_game.player_names_size);
            buffer += event.event_data.new_game.player_names_size;
            break;
        case PIXEL:
            *((msg_event_data_pixel_t*)buffer) = event.event_data.pixel;
            buffer += sizeof(msg_event_data_pixel_t);
            break;
        case PLAYER_ELIMINATED:
            *((msg_event_data_player_eliminated_t*)buffer) =
                event.event_data.player_eliminated;
            buffer += sizeof(msg_event_data_player_eliminated_t);
            break;
    }
    *((uint32_t*)buffer) = event.crc32;
    buffer += sizeof(uint32_t);
    return event.header.len;
}

void send_events(int next_event_no, client_t& client) {
    if (prepared < next_event_no) {
        for (int i = next_event_no; i < events.size(); i++)
            prepare_event(events[i]);
        prepared = events.size() - 1;
    }
    while (next_event_no < events.size()) {
        size_t space_left = MAX_PACKET_SIZE - sizeof(game_id);

        int events_to_send = 0;
        while (events[next_event_no].header.len < space_left) {
            events_to_send++;
            space_left -= events[next_event_no].header.len;
            next_event_no++;
        }

        memset(send_buffer, 0, MAX_PACKET_SIZE);

        *((int*)send_buffer) = htonl(game_id);
        char* buffer = send_buffer + sizeof(game_id);
        for (int i = 0; i < events_to_send; i++)
            buffer +=
                write_to_buffer(buffer, next_event_no - events_to_send + i);

        printf("sending events (%d - %d)\n", next_event_no - events_to_send, next_event_no);
        send_bytes(sock, send_buffer, MAX_PACKET_SIZE - space_left,
                   &client.sockaddr);
    }
}

bool every_Nms(int n) {
    static struct timespec prev = {0, 0};
    static struct timespec next = {0, 0};

    if (clock_gettime(CLOCK_MONOTONIC, &next))
        err("clock_gettime");
    if (next.tv_sec > prev.tv_sec ||
        next.tv_nsec - prev.tv_nsec > n * 1000000) {
        prev = next;
        return true;
    }
    return false;
}

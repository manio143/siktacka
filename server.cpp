#include <vector>
#include <tuple>

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

void make_packet(receive_packet_t* packet_out, arguments_t* args);

uint64_t _random;

void random_init(arguments_t* args) {
    _random = args->random_seed;
}
uint64_t random_next() {
    uint64_t r = _random;
    _random = (_random * 279470273) % 4294967291;
    return r;
}

int client_exists(client_t& client);
void add_client(client_t client);
vector<client_t> clients;

void clean_clients(time_t time) {
    for (int i = 1; i < clients.size(); i++)
        if (time - clients[i].time < 2 * 60) {
            clients[i] = clients.back();
            clients.pop_back();
            //TODO: update player client_index
        }
}

uint32_t game_id = 0;

vector<msg_event_t> events;
int last_event_sent = -1;

vector<player_state_t> players;

int player_name_cmp(player_state_t& p1, player_state_t& p2) {
    return strcmp(p1.player_name, p2.player_name);
}

int* pixels;  // TODO: in main init pixels = new bool[maxx][maxy];

int main(int argc, char** argv) {
    int sock;
    arguments_t args;
    pollfd_t pollfd;

    fill_args(&args, argc, argv);

    random_init(&args);

    init_socket(&pollfd, args.port);
    sock = pollfd.fd;

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
        for (auto& client : clients))
            start = start && client.turn_direction != 0;

        if (!running && start) {
            // GAME INIT
            running = true;
            game_id = random_next();
            for (int i = 0; i < clients.size(); i++) {
                if (strcmp("", client.player_name)) {
                    player_state_t player;
                    player.x = (random_next() % maxx) + 0.5;
                    player.y = (random_next() % maxy) + 0.5;
                    player.direction = random_next() % 360;
                    strcpy(player.player_name, client[i].player_name);
                    player.client_index = i;
                    players.push_back(player);
                }
            }
            sort(players.begin(), players.end(), player_name_cmp);

            events.push_back(make_new_game_event());

            for (auto& player : players) {
                if (pixels[floor(player.x)][floor(player.y)])
                    events.push_back(eliminate_player(player));
                else
                    events.push_back(add_pixel(player));
            }
        }
        if(running) {
            //GAME_UPDATE
            for(int i=0;i<players.size(); i++) {
                auto turn_dir = clients[players[i].client_index].turn_direction;
                if(turn_dir > 0)
                    players[i].direction = (players[i].direction + args.turning_speed) % 360;
                else if(turn_dir < 0) {
                    players[i].direction = (players[i].direction + args.turning_speed) % 360;
                    if(players[i].direction < 0)
                        players[i].direction += 360;
                }

                //TODO: floor_x, floor_y
                //TODO: move x,y towards direction
                //TODO: n_floor_x, n_floor_y
                //if(floor_x == n_floor_x && floor_y == n_floor_y) continue;
                //else repeat condition from line 130

            }
        }
        //TODO: don't update until 1/ROUNDS_PER_SECONDS has passed

        if(active_players == 1) {
            running = false;
        }

        // TODO: wyślij wiadomości
        if (events.size() > last_event_sent + 1) {
            for (auto& client : clients)
                send_events(last_event_sent + 1, client)
        }
    } while (true);

    if (close(sock) == -1)
        err("close");

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
            case 'r':
                args_out->turning_speed = atoi(optarg);
                break;
            case 'r':
                args_out->random_seed = atol(optarg);
                break;
            default:
                err(stderr,
                    "Usage: %s [-W n] [-H n] [-p n] [-s n] [-t n] [-r n]\n",
                    argv[0]);
        }
    }
}

int clients_sock_equal(client_t c1, client_t c2) {
    return c1.sockaddr.sin_port == c2.sockaddr.sin_port &&
           c1.sockaddr.sin_addr.s_addr == c2.sockaddr.sin_addr.s_addr
}
int clients_equal(client_t c1, client_t c2) {
    return clients_sock_equal(c1, c2) && c1.session_id == c2.session_id;
}

int client_exists(client_t client) {
    for (int i = 1; i < clients.size(); i++)
        if (clients_sock_equal(clients[i], client))
            return true;
    return false;
}

bool add_client(client_t client) {
    for (int i = 1; i < clients.size(); i++) {
        if (clients_equal(clients[i], client)) {
            clients[i].time = client.time;
            return true;
        } else if (clients_sock_equal(clients[i], client)) {
            if (client.session_id > clients[i].session_id) {
                clients[i] = client;
                return true;
            } else
                return false;
        }
    }
    clients.push_back(client);
    return true;
}
tuple<bool, int> receive_msg_from_client(pollfd_t& pollfd, client_t& client) {
    client.time = time(NULL);
    msg_from_client_t lpacket;
    memset(lpacket.player_name, 0, sizeof(lpacket.player_name));
    receive_bytes(pollfd.fd, &lpacket, sizeof(receive_packet_t),
                  &(client.sockaddr));
    lpacket = msg_from_client_ntoh(lpacket);
    client.session_id = lpacket.session_id;
    client.turn_direction = lpacket.turn_direction;
    strcpy(client.player_name, lpacket.player_name);
    bool good = true;
    for (auto& client : clients)
        if (!strcmp(client.player_name, lpacket.player_name))
            return make_tuple(false, lpacket);
    good = good && add_client(client);
    return make_tuple(good, lpacket.next_expected_event_no);
}
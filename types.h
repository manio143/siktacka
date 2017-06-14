// types.h

#include <unistd.h>
#include <poll.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>

#ifndef M_TYPES
#define M_TYPES

typedef struct msg_from_client {
	uint64_t session_id;
	uint8_t turn_direction;
	uint32_t next_expected_event_no;
	char player_name[65];
} __attribute__((packed)) msg_from_client_t;

size_t sizeof_msg_from_client(msg_from_client_t * msg) {
    return sizeof(msg->session_id) + sizeof(msg->turn_direction) + sizeof(msg->next_expected_event_no) + strlen(msg->player_name);
}

typedef struct msg_event_header {
    uint32_t len;
    uint32_t event_no;
    uint8_t event_type;
} __attribute__((packed)) msg_event_header_t;

#define msg_event_header_size   sizeof(msg_event_header_t) - sizeof(uint32_t)

typedef struct msg_event_data_new_game {
    uint32_t maxx;
    uint32_t maxy;
    char player_names[MAX_PACKET_SIZE]; //\0 separated names
    size_t player_names_size; //not sent
} __attribute__((packed)) msg_event_data_new_game_t;

typedef struct msg_event_data_pixel {
    uint8_t player_number;
    uint32_t x;
    uint32_t y;    
} __attribute__((packed)) msg_event_data_pixel_t;

typedef struct msg_event_data_player_eliminated {
    uint8_t player_number;
} __attribute__((packed)) msg_event_data_player_eliminated_t;

typedef struct msg_event {
    msg_event_header_t header;
    union {
        msg_event_data_new_game_t new_game;
        msg_event_data_pixel_t pixel;
        msg_event_data_player_eliminated_t player_eliminated;
        void * game_over;
    } event_data;
    uint32_t crc32;    
} __attribute__((packed)) msg_event_t;

typedef struct msg_from_server {
    uint32_t game_id;
    msg_event_t * events;
} __attribute__((packed)) msg_from_server_t;

typedef struct player_state {
    double x;
    double y;
    uint8_t direction;
    bool active;
    int player_no;
    char player_name[65];
    int client_index;
} player_state_t;

typedef struct server_state {
    uint32_t game_id;
    player_state_t * player_states;
    msg_event_t * events;
    uint32_t * pixel_map;
} server_state_t;


typedef struct addrinfo addrinfo_t;
typedef struct sockaddr_in6 sockaddr_t;
typedef struct sockaddr_in sockaddr4_t;
typedef struct pollfd pollfd_t;


typedef struct client {
    sockaddr_t sockaddr;
    time_t time;
    uint64_t session_id;
	uint8_t turn_direction;
    char player_name[65];
    int player_index;
} client_t;

#endif
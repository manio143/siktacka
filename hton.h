// hton.h

#include <arpa/inet.h>
#include "types.h"

#ifndef M_HTON
#define M_HTON

#define htonll(x)    \
    ((1 == htonl(1)) \
         ? (x)       \
         : ((uint64_t)htonl((x)&0xFFFFFFFF) << 32) | htonl((x) >> 32))
#define ntohll(x)    \
    ((1 == ntohl(1)) \
         ? (x)       \
         : ((uint64_t)ntohl((x)&0xFFFFFFFF) << 32) | ntohl((x) >> 32))

msg_from_client_t msg_from_client_hton(msg_from_client_t msg) {
    msg.session_id = htonll(msg.session_id);
    msg.next_expected_event_no = htonl(msg.next_expected_event_no);
    return msg;
}
msg_from_client_t msg_from_client_ntoh(msg_from_client_t msg) {
    msg.session_id = ntohll(msg.session_id);
    msg.next_expected_event_no = ntohl(msg.next_expected_event_no);
    return msg;
}

msg_from_server_t msg_from_server_hton(msg_from_server_t msg) {
    msg.game_id = htonl(msg.game_id);
    return msg;
}
msg_from_server_t msg_from_server_ntoh(msg_from_server_t msg) {
    msg.game_id = ntohl(msg.game_id);
    return msg;
}

msg_event_header_t msg_event_header_hton(msg_event_header_t msg) {
    msg.len = htonl(msg.len);
    msg.event_no = htonl(msg.event_no);
    return msg;
}
msg_event_header_t msg_event_header_ntoh(msg_event_header_t msg) {
    msg.len = ntohl(msg.len);
    msg.event_no = ntohl(msg.event_no);
    return msg;
}



#endif
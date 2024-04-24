#ifndef SERVER_H__
#define SERVER_H__ 1

#include <sys/types.h>

#define OFFSET 3

struct client_info {
    char ID[LGMAX_ID + 1];
    int index;
    int num_topics;
    char **topics;
    u_int8_t connected;
};

//#pragma pack(1)
struct udp_message  {
    char topic[LGMAX_TOPIC + 1];
    uint8_t tip_date;
    char content[LGMAX_VAL + 1];
};

//#pragma pack(1)
struct server_message {
    size_t len;
    struct sockaddr_in udp_addr;
    struct udp_message message;
};

void enroll_client();
void receive_from_client(int index);
void receive_udp();
int client_has_topic(struct client_info *client, struct udp_message *message);

#endif
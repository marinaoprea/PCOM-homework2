#ifndef _SERVER_H
#define _SERVER_H 1

#include <sys/types.h>

#define OFFSET 3

struct client_info {
    char ID[10];
    int index;
    int num_topics;
    char **topics;
    u_int8_t connected;
};

struct udp_message {
    char topic[50];
    uint8_t tip_date;
    char content[LGMAX_VAL];
};

struct server_message {
    struct sockaddr_in udp_addr;
    struct udp_message message;
};

void enroll_client();
void receive_from_client(int index);
void receive_udp();
int client_has_topic(struct client_info *client, struct udp_message *message);

#endif
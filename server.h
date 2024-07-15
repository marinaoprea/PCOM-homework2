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

struct udp_message  {
    char topic[LGMAX_TOPIC + 1];
    uint8_t tip_date;
    char content[LGMAX_VAL + 1];
};

struct server_message {
    size_t len;
    struct sockaddr_in udp_addr;
    struct udp_message message;
};

/**
 * function treats request on listen file descriptor; accepts connection from
 * new client; function recieves client ID and checks if there is another
 * client connected with the same ID; if so, function prints message and closes
 * the connection; otherwise, new client is added in server's database
*/
void enroll_client();

/**
 * function receives request from client on connection socket;
 * if exit request, information about the client is deleted;
 * if subscribe/unsubscribe request client's topic list is updated;
*/
void receive_from_client(int index);

/**
 * fucntion receives message on udp socket, iterates through the clients and
 * sends the message to the ones subscribed to the received topic
*/
void receive_udp();

/**
 * function iterates through client's topics and calls for pattern matching
 * function to check is specific topic is matched
*/
int client_has_topic(struct client_info *client, struct udp_message *message);

/**
 * function frees all resources allocated on heap by server and closes all 
 * communication sockets
*/
void free_resources();

#endif
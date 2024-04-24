#ifndef _SUBSCRIBER_H
#define _SUBSCRIBER_H 1

struct client_message {
    char command[15];
    char topic[LGMAX_TOPIC + 1];
};

void subscribe_request(char *message);

#endif
#ifndef _SUBSCRIBER_H
#define _SUBSCRIBER_H 1

#define MAXLEN 100

struct client_message {
    char command[20];
    char topic[LGMAX_TOPIC];
};

void subscribe_request(char *message);

#endif
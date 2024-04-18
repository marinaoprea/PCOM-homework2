#ifndef _SUBSCRIBER_H
#define _SUBSCRIBER_H 1

#define MAXLEN 100

struct client_message {
    char payload[MAXLEN];
};

#endif
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

void enroll_client();

#endif
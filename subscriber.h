#ifndef _SUBSCRIBER_H
#define _SUBSCRIBER_H 1

struct client_message {
    char command[15];
    char topic[LGMAX_TOPIC + 1];
};

/**
 * function computes and sends to server the subscribe request received from 
 * stdin
*/
void subscribe_request(char *message);

/**
 * function computes and sends to server the unsubscribe request received from
 * stdin
*/
void unsubscribe_request(char *message);

/**
 * function receives message from server, parses it and prints the corresponding
 * outcome
*/
void receive_message();

#endif
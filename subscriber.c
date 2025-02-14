#include <arpa/inet.h>
#include <ctype.h>
#include <errno.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/poll.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <poll.h>
#include <math.h>
#include <netinet/tcp.h>
#include <fcntl.h>

#include "helpers.h"
#include "subscriber.h"
#include "server.h"

int sockfd;

void subscribe_request(char *message) {
    struct client_message client_message;
    memset(&client_message, 0, sizeof(client_message));
    sscanf(message, "%s %s", client_message.command, client_message.topic);

    int rc = send_all(sockfd, (char *)(&client_message), 
                      sizeof(client_message));
    DIE(rc < 0, "send_all");

    printf("Subscribed to topic %s.\n", client_message.topic);

    fflush(stdout);
}

void unsubscribe_request(char *message) {
    struct client_message client_message;
    memset(&client_message, 0, sizeof(client_message));
    sscanf(message, "%s %s", client_message.command, client_message.topic);

    int rc = send_all(sockfd, (char *)(&client_message), 
                      sizeof(client_message));
    DIE(rc < 0, "send_all");

    printf("Unsubscribed from topic %s.\n", client_message.topic);
    fflush(stdout);
}

void receive_message() {
    struct server_message message;
    memset(&message, 0, sizeof(struct server_message));

    int rc = recv_all(sockfd, (char *)(&(message.len)), sizeof(message.len));
    DIE(rc < 0, "recv_all");

    if (message.len == (size_t)(-1)) {
        close(sockfd);
        exit(0);
    }

    char *p = (char *)(&message);
    p += rc;
    rc = recv_all(sockfd, p, message.len - sizeof(message.len));
    DIE(rc < 0, "recv_all");

    printf("%s:%d - %s - ", inet_ntoa(message.udp_addr.sin_addr), 
           ntohs(message.udp_addr.sin_port), message.message.topic);

    char type[15];
    memset(type, 0, sizeof(type));

    if (message.message.tip_date == 0) {
        strcpy(type, "INT");
        uint8_t sign = message.message.content[0];
        uint32_t value = ntohl(*((uint32_t *)((char *)message.message.content 
                                + 1)));
        printf("%s - %d\n", type, (sign ? -1 : 1) * value);
        fflush(stdout);
        return;
    }

    if (message.message.tip_date == 1) {
        strcpy(type, "SHORT_REAL");
        uint16_t value = ntohs(*(uint16_t *)message.message.content);
        printf("%s - %.2f\n", type, value / 100.0);
        fflush(stdout);
        return;
    }
    
    if (message.message.tip_date == 2) {
        strcpy(type, "FLOAT");
        uint8_t sign = message.message.content[0];
        uint32_t value = ntohl(*((uint32_t *)((char *)message.message.content 
                               + 1)));
        uint8_t exp = message.message.content[5];
        printf("%s - %f\n", type, (sign ? -1.0 : 1.0) * value / power10(exp));
        fflush(stdout);
        return;
    }
    if (message.message.tip_date == 3) {
        strcpy(type, "STRING");
        printf("%s - %s\n", type, message.message.content);
        fflush(stdout);
    }
}

int main(int argc, char *argv[]) {
    setvbuf(stdout, NULL, _IONBF,  BUFSIZ); // setting stdout to be unbuffered

    if (argc != 4) {
        printf("\n Usage: %s <ID> <ip> <port>\n", argv[0]);
        return 1;
    }

    // parse port as number
    uint16_t port;
    int rc = sscanf(argv[3], "%hu", &port);
    DIE(rc != 1, "Given port is invalid");

    // socket for connection to the server
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    DIE(sockfd < 0, "socket");

    int enable = 1;
    rc = setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int));
    DIE(rc < 0, "setsockopt");
    rc = setsockopt(sockfd, IPPROTO_TCP, TCP_NODELAY, &enable, sizeof(int));
    DIE(rc < 0, "setsockopt");

    struct sockaddr_in serv_addr;
    socklen_t socket_len = sizeof(struct sockaddr_in);

    // complete address, family address and port
    memset(&serv_addr, 0, socket_len);
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);
    rc = inet_pton(AF_INET, argv[2], &serv_addr.sin_addr.s_addr);
    DIE(rc <= 0, "inet_pton");

    // connect to server
    rc = connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr));
    DIE(rc < 0, "connect");

/////////////////////////////////////////////////////////////////////////

    // creating file descriptor poll structure 
    struct pollfd *poll_fds;
    int num_sockets = 2;

    poll_fds = calloc(2, sizeof(struct pollfd));
    DIE(!poll_fds, "calloc poll_fds");

    poll_fds[0].fd = 0; // stdin
    poll_fds[0].events = POLLIN;

    poll_fds[1].fd = sockfd; // server
    poll_fds[1].events = POLLIN;

    int exit_flag = 0;

    // sending my ID to server
    char my_ID[11];
    memset(my_ID, 0, sizeof(my_ID));
    strcpy(my_ID, argv[1]);
    rc = send_all(sockfd, my_ID, sizeof(my_ID));
    DIE(rc < 0, "send");

    // if connection not accepted, exit program
    char ack;
    rc = recv_all(sockfd, &ack, sizeof(char));
    DIE(rc < 0, "recv_all");
    if (!ack) {
        close(sockfd);
        return 0;
    }

    while(1) {
        rc = poll(poll_fds, num_sockets, -1);
        DIE(rc < 0, "poll");

        for (int i = 0; i < num_sockets; i++) {
            if (poll_fds[i].revents & POLLIN) {
                if (poll_fds[i].fd == 0) { // stdin
                    char message[1024];
                    fgets(message, 1024, stdin);
                    if (strncmp(message, "exit", 4) == 0) {
                        exit_flag = 1;
                        break;
                    }

                    if (strncmp(message, "subscribe", 9) == 0)
                        subscribe_request(message);

                    if (strncmp(message, "unsubscribe", 11) == 0)
                        unsubscribe_request(message);

                } else {
                    if (poll_fds[i].fd == sockfd)
                        receive_message();
                    else
                        fprintf(stderr, "socket unknown %d\n", poll_fds[i].fd);
                }
            }
        }

        if (exit_flag) {
            // announce server about disconnection
            struct client_message message;
            memset(&message, 0, sizeof(message));
            strcpy(message.command, "exit");
            send_all(sockfd, (char *)(&message), sizeof(message));
            DIE(rc < 0, "send_all");
            break;
        }
    }

    rc = close(sockfd);
    DIE(rc < 0, "close");
    free(poll_fds);
    return 0;
}
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

#include "helpers.h"

int main(int argc, char *argv[]) {
    setvbuf(stdout, NULL, _IONBF, 0); // setting stdout to be unbuffered

    if (argc != 4) {
        printf("\n Usage: %s <ID> <ip> <port>\n", argv[0]);
        return 1;
    }

    // Parsam port-ul ca un numar
    uint16_t port;
    int rc = sscanf(argv[3], "%hu", &port);
    DIE(rc != 1, "Given port is invalid");

    // Obtinem un socket TCP pentru conectarea la server
    const int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    DIE(sockfd < 0, "socket");

    // Completăm in serv_addr adresa serverului, familia de adrese si portul
    // pentru conectare
    struct sockaddr_in serv_addr;
    socklen_t socket_len = sizeof(struct sockaddr_in);

    memset(&serv_addr, 0, socket_len);
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);
    rc = inet_pton(AF_INET, argv[2], &serv_addr.sin_addr.s_addr);
    DIE(rc <= 0, "inet_pton");

    // Ne conectăm la server
    rc = connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr));
    DIE(rc < 0, "connect");

/////////////////////////////////////////////////////////////////////////

    struct pollfd *poll_fds;
    int num_sockets = 2;

    poll_fds = calloc(2, sizeof(struct pollfd));
    DIE(!poll_fds, "calloc poll_fds");

    poll_fds[0].fd = 0; // stdin
    poll_fds[0].events = POLLIN;

    poll_fds[1].fd = sockfd; // server
    poll_fds[1].events = POLLIN;

    int exit_flag = 0;

    while(1) {
        rc = poll(poll_fds, num_sockets, -1);
        DIE(rc < 0, "poll");

        for (int i = 0; i < num_sockets; i++) {
            if (poll_fds[i].revents & POLLIN) {
                if (poll_fds[i].fd == 0) { // stdin
                    char message[1024];
                    scanf("%s", message);
                    if (strncmp(message, "exit", 4) == 0) {
                        exit_flag = 1;
                        break;
                    }
                }
            }
        }

        if (exit_flag) {
            break;
        }
    }


    // Inchidem conexiunea si socketul creat
    close(sockfd);
    return 0;
}
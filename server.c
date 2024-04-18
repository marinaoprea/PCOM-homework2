#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/uio.h>

#include "helpers.h"
#include "server.h"
#include "subscriber.h"

int listenfd;

int num_clients;
struct client_info *clients;

struct pollfd *poll_fds;
int num_sockets;

void enroll_client() {
    // Am primit o cerere de conexiune pe socketul de listen, pe care
    // o acceptam
    struct sockaddr_in cli_addr;
    socklen_t cli_len = sizeof(cli_addr);
    const int newsockfd = accept(listenfd, (struct sockaddr *)&cli_addr, &cli_len);
    DIE(newsockfd < 0, "accept");

    // Adaugam noul socket intors de accept() la multimea descriptorilor
    // de citire
    num_sockets++;
    poll_fds = realloc(poll_fds, num_sockets * sizeof(struct pollfd));
    DIE(!poll_fds, "realloc");
    memset(&(poll_fds[num_sockets - 1]), 0, sizeof(struct pollfd));
    poll_fds[num_sockets - 1].fd = newsockfd;
    poll_fds[num_sockets - 1].events = POLLIN;

    char clientID[10];
    memset(clientID, 0, sizeof(clientID));
    int rc = recv(newsockfd, clientID, sizeof(clientID), 0);

    printf("New client %s connected from %s:%d.\n", clientID, inet_ntoa(cli_addr.sin_addr),
            ntohs(cli_addr.sin_port));

    num_clients++;
    clients = realloc(clients, num_clients * sizeof(struct client_info));
    DIE(!clients, "realloc");
    strcpy(clients[num_clients - 1].ID, clientID);
    clients[num_clients - 1].num_topics = 0;
    clients[num_clients - 1].topics = NULL;
    clients[num_clients - 1].connected = 1;
    clients[num_clients - 1].index = num_clients - 1;
}

void receive_from_client(int index) {
    struct client_message message;
    memset(&message, 0, sizeof(message));

    int rc = recv(poll_fds[index].fd, &message, sizeof(message), 0);
    DIE(rc < 0, "recv");

    if (strncmp(message.payload, "disconnected", 12) == 0) {
        clients[index - OFFSET].connected = 0;
        printf("Client %s disconnected.\n", clients[index - OFFSET].ID);
        return;
    }
}

int main(int argc, char *argv[]) {
    // setting stdout to be unbuffered
    setvbuf(stdout, NULL, _IONBF, 0);

    if (argc != 2) {
        printf("\n Usage: %s <port>\n", argv[0]);
        return 1;
    }

    // Parsam port-ul ca un numar
    uint16_t port;
    int rc = sscanf(argv[1], "%hu", &port);
    DIE(rc != 1, "Given port is invalid");


// TCP setup /////////////////////////////////////////////////////////////////////

    // Obtinem un socket TCP pentru receptionarea conexiunilor
    listenfd = socket(AF_INET, SOCK_STREAM, 0);
    DIE(listenfd < 0, "socket");

    // Completăm in serv_addr adresa serverului, familia de adrese si portul
    // pentru conectare
    struct sockaddr_in serv_addr_tcp;
    socklen_t socket_len = sizeof(struct sockaddr_in);

    // Facem adresa socket-ului reutilizabila, ca sa nu primim eroare in caz ca
    // rulam de 2 ori rapid
    int enable = 1;
    if (setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0)
        perror("setsockopt(SO_REUSEADDR) failed");

    memset(&serv_addr_tcp, 0, socket_len);
    serv_addr_tcp.sin_family = AF_INET;
    serv_addr_tcp.sin_port = htons(port);
    /* 0.0.0.0, basically match any IP */
    serv_addr_tcp.sin_addr.s_addr = INADDR_ANY;

    // Asociem adresa serverului cu socketul creat folosind bind
    rc = bind(listenfd, (const struct sockaddr *)&serv_addr_tcp, sizeof(serv_addr_tcp));
    DIE(rc < 0, "bind");


// UDP setup //////////////////////////////////////////////////////////////////////

    int sockfd_udp;
    struct sockaddr_in serv_addr_udp;

    // Creating socket file descriptor
    sockfd_udp = socket(AF_INET, SOCK_STREAM, 0);
    DIE(listenfd < 0, "socket");

    /* Make ports reusable, in case we run this really fast two times in a row */
    enable = 1;
    if (setsockopt(sockfd_udp, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0)
        perror("setsockopt(SO_REUSEADDR) failed");

    // Fill the details on what destination port should the
    // datagrams have to be sent to our process.
    memset(&serv_addr_udp, 0, sizeof(serv_addr_udp));
    serv_addr_udp.sin_family = AF_INET; // IPv4
    /* 0.0.0.0, basically match any IP */
    serv_addr_udp.sin_addr.s_addr = INADDR_ANY;
    serv_addr_udp.sin_port = htons(port);

    // Bind the socket with the server address. The OS networking
    // implementation will redirect to us the contents of all UDP
    // datagrams that have our port as destination
    rc = bind(sockfd_udp, (const struct sockaddr *)&serv_addr_udp, sizeof(serv_addr_udp));
    DIE(rc < 0, "bind failed");

//////////////////////////////////////////////////////////////////////////////////

    // Setam socket-ul listenfd pentru ascultare
    rc = listen(listenfd, 0);
    DIE(rc < 0, "listen");

    num_sockets = 3;

    poll_fds = calloc(3, sizeof(struct pollfd));
    DIE(!poll_fds, "calloc poll_fds");

    poll_fds[0].fd = listenfd;
    poll_fds[0].events = POLLIN;

    poll_fds[1].fd = sockfd_udp;
    poll_fds[1].events = POLLIN;

    poll_fds[2].fd = 0; // stdin to check for exit
    poll_fds[2].events = POLLIN;

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
                    } else ;
                } else {
                    if (poll_fds[i].fd == listenfd) {
                        enroll_client();
                    } else {
                        if (poll_fds[i].fd == sockfd_udp) {
                            ;
                        } else {
                            receive_from_client(i);
                        }
                    }
                }
            }
        }

        if (exit_flag) {
            break;
        }
    }

    close(listenfd);
    close(sockfd_udp);

    free(poll_fds);
    return 0;
}
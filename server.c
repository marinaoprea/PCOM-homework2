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
#include <netinet/tcp.h>

#include "helpers.h"
#include "server.h"
#include "subscriber.h"

int sockfd_udp;
int listenfd;

int num_clients;
struct client_info *clients;

struct pollfd *poll_fds;
int num_sockets;

void enroll_client() {
    // request received on listen socket, we accept it
    struct sockaddr_in cli_addr;
    socklen_t cli_len = sizeof(cli_addr);
    const int newsockfd = accept(listenfd, (struct sockaddr *)&cli_addr, 
                                 &cli_len);
    DIE(newsockfd < 0, "accept");

    int enable = 1;
    int rc = setsockopt(newsockfd, SOL_SOCKET, SO_REUSEADDR , &enable, 
                        sizeof(int));
    DIE(rc < 0, "setsockopt");
    rc = setsockopt(newsockfd, IPPROTO_TCP, TCP_NODELAY , &enable, 
                    sizeof(int));
    DIE(rc < 0, "setsockopt");

    // we receive the client's ID
    char clientID[11];
    memset(clientID, 0, sizeof(clientID));
    rc = recv_all(newsockfd, clientID, sizeof(clientID));
    char ack;

    for (int i = 0; i < num_clients; i++)
        if (strcmp(clientID, clients[i].ID) == 0) {
            if (clients[i].connected == 1) {
                printf("Client %s already connected.\n", clientID);
                ack = 0;
                rc = send_all(newsockfd, &ack, sizeof(char));
                DIE(rc < 0, "send_all");
                rc = close(newsockfd);
                DIE(rc < 0, "close");
                return;
            } else {
                clients[i].connected = 1;
                poll_fds[clients[i].index].fd = newsockfd;

                printf("New client %s connected from %s:%d.\n", clientID,
                       inet_ntoa(cli_addr.sin_addr), ntohs(cli_addr.sin_port));
                ack = 1;
                rc = send_all(newsockfd, &ack, sizeof(char));
                DIE(rc < 0, "send_all");
                return;
            }
        }

    // adding information about the new client
    num_sockets++;
    poll_fds = realloc(poll_fds, num_sockets * sizeof(struct pollfd));
    DIE(!poll_fds, "realloc");
    memset(&(poll_fds[num_sockets - 1]), 0, sizeof(struct pollfd));
    poll_fds[num_sockets - 1].fd = newsockfd;
    poll_fds[num_sockets - 1].events = POLLIN;
    poll_fds[num_sockets - 1].revents = 0;

    printf("New client %s connected from %s:%d.\n", clientID, 
            inet_ntoa(cli_addr.sin_addr), ntohs(cli_addr.sin_port));
    ack = 1;
    rc = send_all(newsockfd, &ack, sizeof(char));
    DIE(rc < 0, "send_all");

    num_clients++;
    clients = realloc(clients, num_clients * sizeof(struct client_info));
    DIE(!clients, "realloc");
    strcpy(clients[num_clients - 1].ID, clientID);
    clients[num_clients - 1].num_topics = 0;
    clients[num_clients - 1].topics = NULL;
    clients[num_clients - 1].connected = 1;
    clients[num_clients - 1].index = num_sockets - 1;
}

void receive_from_client(int index) {
    struct client_message message;
    memset(&message, 0, sizeof(message));

    int rc = recv_all(poll_fds[index].fd, (char *)(&message), sizeof(message));
    DIE(rc < 0, "recv");

    if (strncmp(message.command, "exit", 4) == 0) {
        clients[index - OFFSET].connected = 0;
        close(poll_fds[index].fd);
        printf("Client %s disconnected.\n", clients[index - OFFSET].ID);
        return;
    }

    if (strncmp(message.command, "subscribe", 9) == 0) {
        int index_client = index - OFFSET;
        struct client_info *client = &(clients[index_client]);

        client->num_topics++;
        client->topics = realloc(client->topics, 
                                 client->num_topics * sizeof(char *));
        DIE(!client->topics, "realloc");

        client->topics[client->num_topics - 1] = malloc((LGMAX_TOPIC + 1) 
                                                         * sizeof(char));
        DIE(!client->topics[client->num_topics - 1], "malloc");
        memset(client->topics[client->num_topics - 1], 0, LGMAX_TOPIC + 1);
        strcpy(client->topics[client->num_topics - 1], message.topic);
        return;
    }

    if (strncmp(message.command, "unsubscribe", 11) == 0) {
        int index_client = index - OFFSET;
        struct client_info *client = &(clients[index_client]);

        int topic_index = -1;
        for (int i = 0; i < client->num_topics; i++) {
            if (strcmp(client->topics[i], message.topic) == 0) {
                topic_index = i;
                break;
            }
        }

        if (topic_index == -1) {
            fprintf(stderr, "Client was not subscribed to topic %s.\n", 
                    message.topic);
            return;
        }

        // free topics, then move pointers to the left
        free(client->topics[topic_index]);
        for (int i = topic_index; i < client->num_topics - 1; i++)
            client->topics[i] = client->topics[i + 1];

        client->num_topics--;
        if (client->num_topics == 0)
            free(client->topics);
        else {
            client->topics = realloc(client->topics, 
                                     client->num_topics * sizeof(char *));
            DIE(!client->topics, "realloc");
        }
        return;
    }
}

int client_has_topic(struct client_info *client, struct udp_message *message) {
    for (int i = 0; i < client->num_topics; i++)
        if (pattern_matching(client->topics[i], message->topic))
            return 1;
    return 0;
}

void receive_udp() {
    char *buf = malloc(sizeof(struct udp_message));
    DIE(!buf, "malloc");
    memset(buf, 0, sizeof(struct udp_message));

    struct sockaddr_in client_addr;
    socklen_t clen = sizeof(client_addr);
    int rc = recvfrom(sockfd_udp, buf, sizeof(struct udp_message), 0, 
                      (struct sockaddr *)&client_addr, &clen);
    DIE(rc < 0, "recvfrom");

    struct server_message server_msg;
    memset(&server_msg, 0, sizeof(struct server_message));

    memcpy(&(server_msg.message.topic), buf, LGMAX_TOPIC);
    memcpy(&(server_msg.message.tip_date), buf + LGMAX_TOPIC, 1);
    memcpy(&(server_msg.message.content), buf + LGMAX_TOPIC + 1, LGMAX_VAL);

    memcpy(&(server_msg.udp_addr), &client_addr, sizeof(client_addr));

    server_msg.len = sizeof(struct server_message) - 
                     sizeof(server_msg.message.content);
    if (server_msg.message.tip_date == 0)
        server_msg.len += 5;
    else
        if (server_msg.message.tip_date == 1)
            server_msg.len += 2;
        else
            if (server_msg.message.tip_date == 2)
                server_msg.len += 6;
            else
                server_msg.len += strlen(server_msg.message.content);

    for (int i = 0; i < num_clients; i++)
        if (client_has_topic(&(clients[i]), &(server_msg.message))) {
            send_all(poll_fds[clients[i].index].fd, (char *)(&server_msg),
                     server_msg.len);
        }

    free(buf);
}

void free_resources() {
    int rc = close(listenfd);
    DIE(rc < 0, "close");
    rc = close(sockfd_udp);
    DIE(rc < 0, "close");
    rc = close(0);
    DIE(rc < 0, "close");
    size_t exit_fl = (size_t)(-1);
    for (int i = 3; i < num_sockets; i++) {
        rc = send_all(poll_fds[i].fd, (char *)(&exit_fl), sizeof(size_t));
        DIE(rc < 0, "send_all");
        rc = close(poll_fds[i].fd);
        DIE(rc < 0, "close");
    }

    free(poll_fds);
    for (int i = 0; i < num_clients; i++) {
        if(clients[i].num_topics) {
            for (int j = 0; j < clients[i].num_topics; i++)
                free(clients[i].topics[j]);
            free(clients[i].topics);
        }
    }
    free(clients);
}

int main(int argc, char *argv[]) {
    // setting stdout to be unbuffered
    setvbuf(stdout, NULL, _IONBF, BUFSIZ);

    if (argc != 2) {
        printf("\n Usage: %s <port>\n", argv[0]);
        return 1;
    }

    // parse port as number
    uint16_t port;
    int rc = sscanf(argv[1], "%hu", &port);
    DIE(rc != 1, "Given port is invalid");

// TCP setup /////////////////////////////////////////////////////////////////
    listenfd = socket(AF_INET, SOCK_STREAM, 0);
    DIE(listenfd < 0, "socket");

    int enable = 1;

    struct sockaddr_in serv_addr_tcp;
    socklen_t socket_len = sizeof(struct sockaddr_in);

    enable = 1;
    rc = setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int));
    DIE(rc < 0, "setsockopt");
    rc = setsockopt(listenfd, IPPROTO_TCP, TCP_NODELAY, &enable, sizeof(int));
    DIE(rc < 0, "setsockopt");

    // complete server address, family address and port
    memset(&serv_addr_tcp, 0, socket_len);
    serv_addr_tcp.sin_family = AF_INET;
    serv_addr_tcp.sin_port = htons(port);
    serv_addr_tcp.sin_addr.s_addr = INADDR_ANY;

    // bind the socket with the server address
    rc = bind(listenfd, (const struct sockaddr *)&serv_addr_tcp, 
              sizeof(serv_addr_tcp));
    DIE(rc < 0, "bind");

// UDP setup //////////////////////////////////////////////////////////////////

    struct sockaddr_in serv_addr_udp;

    sockfd_udp = socket(AF_INET, SOCK_DGRAM, 0);
    DIE(sockfd_udp < 0, "socket");

    enable = 1;
    rc = setsockopt(sockfd_udp, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int));
    DIE(rc < 0, "setsockopt");

    // filling the details on what destination port should the
    // datagrams have to be sent to our process.
    memset(&serv_addr_udp, 0, sizeof(serv_addr_udp));
    serv_addr_udp.sin_family = AF_INET; // IPv4
    serv_addr_udp.sin_addr.s_addr = INADDR_ANY;
    serv_addr_udp.sin_port = htons(port);

    // bind the socket with the server address
    rc = bind(sockfd_udp, (const struct sockaddr *)&serv_addr_udp, 
              sizeof(serv_addr_udp));
    DIE(rc < 0, "bind failed");

///////////////////////////////////////////////////////////////////////////////

    // set socket for listening
    rc = listen(listenfd, 0);
    DIE(rc < 0, "listen");

    // creating file descriptor poll structure
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
                    fgets(message, 1024, stdin);
                    if (strncmp(message, "exit", 4) == 0) {
                        exit_flag = 1;
                        break;
                    } else ;
                } else {
                    if (poll_fds[i].fd == listenfd)
                        enroll_client();
                    else
                        if (poll_fds[i].fd == sockfd_udp)
                            receive_udp();
                        else
                            receive_from_client(i);
                }
            }
        }

        if (exit_flag)
            break;
    }

    free_resources();

    return 0;
}
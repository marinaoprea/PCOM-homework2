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
    const int listenfd = socket(AF_INET, SOCK_STREAM, 0);
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

    struct pollfd *poll_fds;
    int num_sockets = 2;

    poll_fds = calloc(2, sizeof(struct pollfd));
    poll_fds[0].fd = listenfd;
    poll_fds[0].events = POLLIN;

    poll_fds[1].fd = sockfd_udp;
    poll_fds[1].events = POLLIN;

    while(1);

    close(listenfd);
    close(sockfd_udp);
    return 0;
}
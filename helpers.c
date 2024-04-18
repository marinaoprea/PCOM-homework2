#include "helpers.h"
#include <stdio.h>
#include <sys/socket.h>

int recv_all(int sockfd, void *buffer, size_t len) {
  size_t bytes_received = 0;
  size_t bytes_remaining = len;
  char *buff = buffer;

  while (bytes_remaining) {
    bytes_received = recv(sockfd, buff, bytes_remaining, 0);
    
    if (bytes_received < 0) {
        return -1;
    }
    if (bytes_received == 0)
        return bytes_received;
    bytes_remaining -= bytes_received;
    buff += bytes_received;
  }
  return len;
}

/*
    TODO 1.2: Rescrieți funcția de mai jos astfel încât ea să facă trimiterea
    a exact len octeți din buffer.
*/

int send_all(int sockfd, void *buffer, size_t len) {
  size_t bytes_sent = 0;
  size_t bytes_remaining = len;
  char *buff = buffer;

  while (bytes_remaining) {
    bytes_sent = send(sockfd, buff, bytes_remaining, 0);
    if (bytes_sent < 0)
      return bytes_sent;

    bytes_remaining -= bytes_sent;
    buff += bytes_sent;
  }
  return len;
}
#include "helpers.h"
#include <stdio.h>
#include <sys/socket.h>
#include <string.h>

int recv_all(int sockfd, char *buffer, size_t len) {
  ssize_t bytes_received = 0;
  ssize_t bytes_remaining = len;
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

int send_all(int sockfd, char *buffer, size_t len) {
  ssize_t bytes_sent = 0;
  ssize_t bytes_remaining = len;
  char *buff = buffer;

  while (bytes_remaining) {
    bytes_sent = send(sockfd, buff, bytes_remaining, 0);
    if (bytes_sent < 0)
      return bytes_sent;
    if (bytes_sent == 0)
      return bytes_sent;

    bytes_remaining -= bytes_sent;
    buff += bytes_sent;
  }
  return len;
}

int pattern_matching(char *str1, char *str2) {
    if (str1 == NULL && str2 == NULL)
      return 1;
    if (str1 == NULL)
      return 0;
    if (str2 == NULL)
      return 0;

    char *p1 = strtok(str1, "/");
    char *p2 = strtok(str2, "/");

    if (p1 == NULL || p2 == NULL)
        return (strcmp(str1, str2) == 0);

    char *next1 = str1 + strlen(p1) + 1;
    char *next2 = str2 + strlen(p2) + 1;
    if (strcmp(p1, p2) == 0 || strcmp(p1, "+") == 0 || strcmp(p2, "+") == 0)
        return pattern_matching(next1, next2);
    
    if (strcmp(p1, "*") == 0) {
       int ans = 0;
       while (p2 && !ans) {
          ans |= pattern_matching(next1, next2);
          p2 = strtok(NULL, "/");
          if (p2)
            next2 += strlen(p2) + 1;
       }

       return ans;
    }

    if (strcmp(p2, "*") == 0) {
       int ans = 0;
       while (p1 && !ans) {
          ans |= pattern_matching(next1, next2);
          p1 = strtok(NULL, "/");
          if (p1)
            next1 += strlen(p1) + 1;
       }

       return ans;
    }

    return 0;
}
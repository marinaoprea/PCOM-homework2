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
    int n1 = strlen(str1);
    int n2 = strlen(str2);

    char *aux1 = malloc(1 + LGMAX_TOPIC);
    DIE(!aux1, "malloc");
    char *aux2 = malloc(1 + LGMAX_TOPIC);
    DIE(!aux2, "malloc");
    memcpy(aux1, str1, n1 + 1);
    memcpy(aux2, str2, n2 + 1);

    char **rest1 = &aux1;
    char **rest2 = &aux2;

    // copy of ponters in order to free memory correctly after tokenization
    char *copy1 = aux1;
    char *copy2 = aux2;

    // using reentrant variant of strtok in order to tokenize both strings
    char *p1 = __strtok_r(aux1, "/", rest1);
    char *p2 = __strtok_r(aux2, "/", rest2);

    if (p1 == NULL || p2 == NULL) {
      free(copy1);
      free(copy2);
      return (strcmp(str1, str2) == 0);
    }

    int star1 = 0;
    int star2 = 0;
    while (p1 && p2) {
        if (strcmp(p1, p2) == 0 || strcmp(p1, "+") == 0 || 
            strcmp(p2, "+") == 0) {
          star1 = 0;
          star2 = 0;
          p1 = __strtok_r(NULL, "/", rest1);
          p2 = __strtok_r(NULL, "/", rest2);
          continue;
        }

        if (strcmp(p1, "*") == 0) {
          p1 = __strtok_r(NULL, "/", rest1);
          star1 = 1;
          continue;
        }

        if (strcmp(p2, "*") == 0) {
          p2 = __strtok_r(NULL, "/", rest1);
          star1 = 1;
          continue;
        }

        if (strcmp(p1, p2)) {
          // if not equal, try to pattern match with *
          if (star1) {
              p2 = __strtok_r(NULL, "/", rest2);
              continue;      
          }
          if (star2) {
              p1 = __strtok_r(NULL, "/", rest1);
              continue;   
          }
          break;
        }
    }

    free(copy1);
    free(copy2);
    if (!p1 && !p2)
      return 1;
    
    // try to pattern match the rest with * in str2
    if (p1)
      return star2;
    // try to pattern match the rest with * in str1
    return star1;
}

double power10(uint8_t exp) {
    double ans = 1;
    while (exp--)
        ans *= 10;
    return ans;
}
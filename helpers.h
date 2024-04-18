#ifndef _HELPERS_H
#define _HELPERS_H 1

#include <stdio.h>
#include <stdlib.h>

#define LGMAX_ID 10
#define LGMAX_TOPIC 50

/*
 * Macro de verificare a erorilor
 * Exemplu:
 * 		int fd = open (file_name , O_RDONLY);
 * 		DIE( fd == -1, "open failed");
 */

#define DIE(assertion, call_description)                                       \
  do {                                                                         \
    if (assertion) {                                                           \
      fprintf(stderr, "(%s, %d): ", __FILE__, __LINE__);                       \
      perror(call_description);                                                \
      exit(EXIT_FAILURE);                                                      \
    }                                                                          \
  } while (0)

int recv_all(int sockfd, void *buffer, size_t len);
int send_all(int sockfd, void *buffer, size_t len);

#endif

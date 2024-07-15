#ifndef _HELPERS_H
#define _HELPERS_H 1

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#define LGMAX_ID 10
#define LGMAX_TOPIC 50
#define LGMAX_VAL 1500

#define DIE(assertion, call_description)                                       \
  do {                                                                         \
    if (assertion) {                                                           \
      fprintf(stderr, "(%s, %d): ", __FILE__, __LINE__);                       \
      perror(call_description);                                                \
      exit(EXIT_FAILURE);                                                      \
    }                                                                          \
  } while (0)

/**
 * function calls recv system call repeatedly until all len bytes are received
*/
int recv_all(int sockfd, char *buffer, size_t len);

/**
 * function calls send system call repeatedly until all len bytes are sent
*/
int send_all(int sockfd, char *buffer, size_t len);

/**
 * function matches given strings; strings may contain "*" or "+" characters
*/
int pattern_matching(char *str1, char *str2);

/**
 * function calculates 10 ^ exp
*/
double power10(uint8_t exp);

#endif

CFLAGS = -Wall -g -Werror -Wno-error=unused-variable
CC = gcc

all: server subscriber

helpers.o: helpers.c

server: server.c helpers.o

subscriber: subscriber.c helpers.o

clean:
	rm -rf server subscriber *.o 
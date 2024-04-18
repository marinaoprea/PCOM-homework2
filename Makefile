CFLAGS = -Wall -g -Werror -Wno-error=unused-variable

all: server subscriber

server: server.c

subscriber: subscriber.c

clean:
	rm -rf server subscriber *.o 
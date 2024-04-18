CFLAGS = -Wall -g -Werror -Wno-error=unused-variable

server: server.c

subscriber: subscriber.c

clean:
	rm -rf server subscriber *.o 
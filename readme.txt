    Current solution proposes an implementation of an app layer protocol over
tcp, through which clients receive messages from server on the topics they are
subscribed to.

    Protocol rules:
-> after connection, a client needs to send its unique ID to the server
-> the server shall send an aknowledgement, with the following meaning:
    char ack = 0 -> connection established, client enrolled/reconnected (in 
                    case of reconnection, server shall open a new socket and
                    save it in the database)
    char ack = 1 -> connection not established because ID is already used
-> clients shall receive commands from standard input and send them to the
   server packed in the form of "struct client_message", containing the
   command type and command parameter:
    -> exit: client shall inform the server that it will close connection
             server shall set client in database as disconnected and shall close
             connection
    -> subscribe: server shall add the new topic to the client's list of
                  interest
    -> unsubscribe: server shall remove the topic from the client's list of
                    interest
-> server shall receive messages as datagrams (interpreted as "struct
   udp_message") and send them to all interested clients; server shall 
   calculate the total length of useful data and shall send exactly that number
   of bytes in order to avoid congestion due to unnecessary transfers;
   the information is packed in the form of "struct server_message"; that is,
   the first information sent is the length of the useful data, then data
   itself; data encapsulates both the information about the udp client 
   (address and port) and the udp message itself in order to be later 
   interpreted by the client; client firsly receives data length and thus it
   knows exactly how much data it has to receive later on
-> when server receives "exit" command from standard input, it acknowledges all
   clients to close the connections by sending them a data length value of 
   0xFFFFFFFF.

Data structures:
-> for messages:
    -> struct client_message: command, eventual parameter (in case of subscribe
       or unsubscribe commands)
    -> struct udp_message: contains message topic, type and content
    -> struct server_message: length of useful data, struct sockaddr_in 
       containing information about the udp client that sent data and struct 
       udp_message as field containing actual data
    -> struct client_info: ID, index in the poll_fds table, number of topics and
       topics array, connection status (topics' matrix was allocated on heap, 
       in order to be easily resized; moreover the lines have variable size in
       order to save memory)
    -> struct pollfd arrays, dinamically allocated in order to be easily resized
       containing the list of connections through sockets' file descriptors

-> The correlation between the index in the server's poll_fds table and the
    index in the client information database can be done by using the OFFSET
    3, given by the first 3 file descriptors that have no corresponding tcp
    clients: listening file descriptor, udp file descriptor and standard input
    file descriptor.
-> Nagle algorithm was disabled in order for short messages to be sent 
   immediately.
-> Clients receive messages only when they are connected and only on the topics
   they are subscribed to.
-> I/O multiplexing was used both for the server and the clients in order to 
parse events chronologically and efficiently.

Pattern matching:
-> server pattern matches the received messages topics' with the clients' 
subscribed topics
-> strings are tokenized using reentrant variant of strtok in order to work
   on both strings at the same time
-> in case of equal tokens or "+" character, the algorithm moves further on;
   in case of unequal tokens, the algorithm checks if a "*" character was 
   previously found in order to match current levels
-> if there are no tokens formed, match is given by simple strcmp function
-> for every udp message, a client receives it only at most once, regardless 
   of how many of its subscribed topics match the given topic
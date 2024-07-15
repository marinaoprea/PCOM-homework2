<h3>Overview</h3>
    Current solution proposes an implementation of an app layer protocol over
tcp, through which clients receive messages from server on the topics they are
subscribed to.

<h3>Protocol rules</h3>
<ul>
<li>after connection, a client needs to send its unique ID to the server</li>
<li>the server shall send an aknowledgement, with the following meaning:
    <ul>
    <li>
    <b><i>char ack = 0</i></b> -> connection established, client enrolled/reconnected (in 
                    case of reconnection, server shall open a new socket and
                    save it in the database)
    </li>
    <li>
    <b><i>char ack = 1 </i></b>-> connection not established because ID is already used
    </li>
    </ul>
</li>
<li>clients shall receive commands from standard input and send them to the
   server packed in the form of "struct client_message", containing the
   command type and command parameter:</li>
    <ul>
    <li><b><i>exit</i></b>: client shall inform the server that it will close connection;
             server shall set client in database as disconnected and shall close
             connection</li>
    <li> <b><i>subscribe</i></b>: server shall add the new topic to the client's list of
                  interest</li>
    <li> <b><i>unsubscribe</i></b>: server shall remove the topic from the client's list of
                    interest</li>
    </ul>
<li> server shall receive messages as datagrams (interpreted as "struct
   udp_message") and send them to all interested clients; server shall 
   calculate the total length of useful data and shall send exactly that number
   of bytes in order to avoid congestion due to unnecessary transfers;
   the information is packed in the form of "struct server_message"; that is,
   the first information sent is the length of the useful data, then data
   itself; data encapsulates both the information about the udp client 
   (address and port) and the udp message itself in order to be later 
   interpreted by the client; client firsly receives data length and thus it
   knows exactly how much data it has to receive later on </li>
<li>when server receives "exit" command from standard input, it acknowledges all
   clients to close the connections by sending them a data length value of 
   0xFFFFFFFF.</li>
</ul>

<h3>Data structures</h3>
<ul>
<li> for messages:
    <ul>
    <li> <b><i>struct client_message</i></b>: command, eventual parameter (in case of subscribe
       or unsubscribe commands) </li>
    <li> <b><i>struct udp_message</i></b>: contains message topic, type and content</li>
    <li> <b><i>struct server_message</i></b>: length of useful data, struct sockaddr_in 
       containing information about the udp client that sent data and struct 
       udp_message as field containing actual data </li>
    <li> <b><i>struct client_info</i></b>: ID, index in the poll_fds table, number of topics and
       topics array, connection status (topics' matrix was allocated on heap, 
       in order to be easily resized; moreover the lines have variable size in
       order to save memory) </li>
    <li> <b><i>struct pollfd arrays</i></b>, dinamically allocated in order to be easily resized
       containing the list of connections through sockets' file descriptors </li>
    </ul>
</li>
    
<li> The correlation between the index in the server's poll_fds table and the
    index in the client information database can be done by using the OFFSET
    3, given by the first 3 file descriptors that have no corresponding tcp
    clients: listening file descriptor, udp file descriptor and standard input
    file descriptor.
</li>
<li>Nagle algorithm was disabled in order for short messages to be sent 
   immediately.
</li>
<li> Clients receive messages only when they are connected and only on the topics
   they are subscribed to.
</li>
<li>I/O multiplexing was used both for the server and the clients in order to 
parse events chronologically and efficiently.
</li>

</ul>

<h3>Pattern matching</h3>
<ul>
<li> server pattern matches the received messages topics' with the clients' 
subscribed topics </li>
<li> strings are tokenized using reentrant variant of <b><i>strtok</i></b> in order to work
   on both strings at the same time </li>
<li> in case of equal tokens or "+" character, the algorithm moves further on;
   in case of unequal tokens, the algorithm checks if a "*" character was 
   previously found in order to match current levels </li>
<li> if there are no tokens formed, match is given by simple strcmp function </li>
<li> for every udp message, a client receives it only at most once, regardless 
   of how many of its subscribed topics match the given topic</li>
</ul>

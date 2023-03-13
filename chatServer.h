#ifndef CHAT_SERVER_H
#define CHAT_SERVER_H

#include <sys/select.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <signal.h>
#include <netdb.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>

#define IS_VALID_SOCKET(s) ((s) >= 0) // On UNIX, returns a negative number on failure
#define CLOSE_SOCKET(s) close(s)
#define SOCKET int

#define	SUCCESS	0
#define FAILURE	-1

#define MIN_PORT_NUM 1
#define MAX_PORT_NUM 65536 // that is 2^16 = 65,536

#define CMDLINE_ARG_NUM 2
#define LISTEN_BACK_LOG 10
#define TIME_OUT_VAL 120	// This is the time out in seconds (60 * 2 = 120secs) i.e. 2minutes

#define BUFFER_SIZE 4096

/*
 * Data structure to keep track of messages. Each message object holds one
 * complete line of message from a client.  
 *
 * The message objects are maintained per connection in a doubly-linked list. 
 * When a message is read from one connection, it is added to the list of all other connections.
 *
 * A message is added to the list only when a complete line has been read from
 * the client. 
 */
typedef struct msg {
        /* Points to the previous message object in the doubly-linked list. */
        struct msg *prev;
        /* Points to the next message object in the doubly-linked list. */
        struct msg *next;
        /* Points to a dynamically allocated buffer holding the message. */
        char *message;
        /* Size of the message. */
        int size;
}msg_t;


/*
 * Data structure to keep track of client connection state.
 *
 * The connection objects are also maintained in a global doubly-linked list.
 * There is a dummy connection head at the beginning of the list.
 */
typedef struct conn {
        /* Points to the previous connection object in the doubly-linked list. */
        struct conn *prev;      
        /* Points to the next connection object in the doubly-linked list. */
        struct conn *next;      
        /* File descriptor associated with this connection. */
        int fd;                 
        /* 
         * Pointers for the doubly-linked list of messages that
         * have to be written out on this connection.
         */
        msg_t *write_msg_head;
		msg_t *write_msg_tail;
}conn_t;


/* 
 * Data structure to keep track of active client connections (not for the main socket).
 */
typedef struct conn_pool { 
        /* Largest file descriptor in this pool. */
        int maxfd;                      
        /* Number of ready descriptors returned by select. */
        int nready;                     
        /* Set of all active descriptors for reading. */
        fd_set read_set;                
        /* Subset of descriptors ready for reading. */
        fd_set ready_read_set;  
        /* Set of all active descriptors for writing. */
        fd_set write_set;               
        /* Subset of descriptors ready for writing.  */
        fd_set ready_write_set; 
        /* Doubly-linked list of active client connection objects. */
        conn_t *conn_head;
        conn_t *conn_tail;
        /* Number of active client connections. */
        unsigned int nr_conns;
        
}conn_pool_t;

/*
 * Init the conn_pool_t structure. 
 * @pool - allocated pool 
 * @ return value - 0 on success, -1 on failure 
 */
int init_pool(conn_pool_t* pool);

/*
 * Add connection when new client connects the server. 
 * @ sd - the socket descriptor returned from accept
 * @pool - the pool 
 * @ return value - 0 on success, -1 on failure 
 */
int add_conn(int sd, conn_pool_t* pool);

/*
 * Remove connection when a client closes connection, or clean memory if server stops. 
 * @ sd - the socket descriptor of the connection to remove
 * @pool - the pool 
 * @ return value - 0 on success, -1 on failure 
 */
int remove_conn(int sd, conn_pool_t* pool);

/*
 * Add msg to the queues of all connections (except of the origin). 
 * @ sd - the socket descriptor to add this msg to the queue in its conn object
 * @ buffer - the msg to add
 * @ len - length of msg
 * @pool - the pool 
 * @ return value - 0 on success, -1 on failure 
 */
int add_msg(int sd,char* buffer,int len,conn_pool_t* pool);


/*
 * Write msg to client. 
 * @ sd - the socket descriptor of the connection to write msg to
 * @pool - the pool 
 * @ return value - 0 on success, -1 on failure 
 */
int write_to_client(int sd,conn_pool_t* pool);

#endif

/* Description: An event-driven chat server implemented in the C programming
 * language. Its function is to forward any incoming message to all clients
 * except for the client over which the message was received.
 */

#include "chatServer.h"

static int end_server = 0;

fd_set master, slave;

void intHandler(int SIG_INT) 
{
	/* use a flag to end_server to break the main loop */
	end_server = 1;
}

void help_msg()
{
	printf("NOTICE!!\n");
	printf("\tMake sure your provided port number is within the range\n");
	printf("\tof 1 to 65,536\n\n");
}

int main (int argc, char *argv[])
{
	int port = atoi(argv[1]);
		
	if((argc < CMDLINE_ARG_NUM ) || ((port <= (MIN_PORT_NUM -1)) || (port > MAX_PORT_NUM)))
	{
		help_msg();
		printf("Usage: server <port>");
		
		exit(FAILURE);
	}
	
	char *port_num = argv[1];
	
	signal(SIGINT, intHandler);
   
	conn_pool_t* pool = (conn_pool_t*)malloc(sizeof(conn_pool_t));
	
	// If malloc was unable to allocate memory for use. Stop the program and
	// return -1 as FAILURE.
	if(pool == NULL)
	{
		perror("Error: Malloc failure!.");
		exit(FAILURE);
	}
	
	// Initialize the pool of connections. 
	init_pool(pool);
	
	/*************************************************************/
	/* Create an AF_INET stream socket to receive incoming      */
	/* connections on                                            */
	/*************************************************************/
	printf("Configuring local address...\n");
	
    struct addrinfo info;
    memset(&info, 0, sizeof(info));
    info.ai_family = AF_INET;
    info.ai_socktype = SOCK_STREAM;
    info.ai_flags = AI_PASSIVE;

    struct addrinfo *bind_address;
    getaddrinfo(0, port_num, &info, &bind_address);

    printf("Creating socket...\n");
    SOCKET socket_listen;
    socket_listen = socket(bind_address->ai_family, bind_address->ai_socktype, bind_address->ai_protocol);
    
    if (!IS_VALID_SOCKET(socket_listen)) 
    {
        perror("socket creation failed!.\n");
        return FAILURE;
    }
   
	/*************************************************************/
	/* Set socket to be nonblocking. All of the sockets for      */
	/* the incoming connections will also be nonblocking since   */
	/* they will inherit that state from the listening socket.   */
	/*************************************************************/
	//ioctl(...);
	// For switching on non-blocking functionality of sockets.
	int on = 1, rc = 0;
	
	// Allow socket descriptor to be reusable by setting it using the setsockopt() function.
	rc = setsockopt(socket_listen, SOL_SOCKET, SO_REUSEADDR, (char*)&on, sizeof(on));
	
	if(rc < 0)
	{
		perror("The function \'setsockopt()\' failed!");
		exit(FAILURE);
	}
	
	rc = ioctl(socket_listen, (int)FIONBIO, (char*)&on);
	
	if(rc < 0)
	{
		perror("The function \'ioctl()\' failed!.");
		exit(FAILURE);
	}
	
	/*************************************************************/
	/* Bind the socket                                           */
	/*************************************************************/
	printf("Binding socket to local address...\n");
    
    if (bind(socket_listen, bind_address->ai_addr, bind_address->ai_addrlen)) 
    {
        perror("bind() failed!.\n");
        exit(FAILURE);
    }
    freeaddrinfo(bind_address);

	/*************************************************************/
	/* Set the listen back log                                   */
	/*************************************************************/
	printf("Listening...\n");
    
    if (listen(socket_listen, LISTEN_BACK_LOG) < 0) 
    {
        perror("listen() failed!.\n");
        exit(FAILURE);
    }

	/*************************************************************/
	/* Initialize fd_sets  			                             */
	/*************************************************************/
	//fd_set master, slave;
	FD_ZERO(&master);
	pool->maxfd = socket_listen;
	FD_SET(socket_listen, &master);
	
	struct timeval time_out;
	time_out.tv_sec = TIME_OUT_VAL;
	time_out.tv_usec = 0;
	char recv_buffer[BUFFER_SIZE];
	
	int hold_fd = pool->maxfd;

	/*************************************************************/
	/* Loop waiting for incoming connects, for incoming data or  */
	/* to write data, on any of the connected sockets.           */
	/*************************************************************/
	do
	{
		/**********************************************************/
		/* Copy the master fd_set over to the working fd_set.     */
		/**********************************************************/
		memcpy(&slave, &master, sizeof(master));
		/**********************************************************/
		/* Call select() 										  */
		/**********************************************************/
		printf("Waiting on select()...\nMaxFD: %d\n", hold_fd);
		
		int select_return_desc = select((pool->maxfd) + 1, &slave, NULL, NULL, &time_out);
		
		// Select returns FAILURE(-1) when it's unsuccessful, 0 when time limit expires and n 
		// which stands for the total number of descriptors that met the selection criteria
		if(select_return_desc < 0)
		{
			perror("select() failed!.\n");
			break;
		}
		else if((select_return_desc != 0) || (select_return_desc != FAILURE))
		{
			pool->nready = select_return_desc;
		}
		else
		{
			printf("Select() timed out!.\n");
			break;
		}
		
		/**********************************************************/
		/* One or more descriptors are readable or writable.      */
		/* Need to determine which ones they are.                 */
		/**********************************************************/
		SOCKET desc;
		
		int read_set = select_return_desc;
		FD_SET(read_set, &pool->read_set);
		
		for(desc = 1; desc <= (pool->maxfd) && read_set > 0; ++desc)
		{
			/* Each time a ready descriptor is found, one less has  */
			/* to be looked for.  This is being done so that we     */
			/* can stop looking at the working set once we have     */
			/* found all of the descriptors that were ready         */
				
			/*******************************************************/
			/* Check to see if this descriptor is ready for read   */
			/*******************************************************/
			if(FD_ISSET(desc, &slave)) 
			{
			   /***************************************************/
				/* A descriptor was found that was readable		   */
				/* if this is the listening socket, accept one      */
				/* incoming connection that is queued up on the     */
				/*  listening socket before we loop back and call   */
				/* select again. 						            */
				/****************************************************/
				
				FD_SET((read_set - 1), &pool->ready_read_set);
				--read_set;
				
                if (desc == socket_listen) 
                {
                    struct sockaddr_storage client_address;
                    socklen_t client_len = sizeof(client_address);
                    SOCKET new_client = accept(socket_listen, (struct sockaddr*) &client_address, &client_len);
                    
                    if (!IS_VALID_SOCKET(new_client)) 
                    {
                        perror("Accept() failed");
                        end_server = 1;
                    }
                    
                    printf("New incoming connection on sd: %d\n", new_client);
                   
                    add_conn(new_client, pool);
                    pool->nr_conns += 1;
					
                    FD_SET(new_client, &master);
                    if (new_client > (pool->maxfd))
                        pool->maxfd = new_client;
                    hold_fd = pool->maxfd;
                } 
                
                /****************************************************/
				/* If this is not the listening socket, an 			*/
				/* existing connection must be readable				*/
				/* Receive incoming data his socket             */
				/****************************************************/
                else 
                {
                    char recv_buffer[BUFFER_SIZE];
 
                    int received_data = recv(desc, recv_buffer, sizeof(recv_buffer), 0);
                             
                    FD_SET(received_data, &pool->write_set);
                    
                    if (received_data == 0 || received_data < 0) 
                    {
                        printf("Connection closed for sd: %d.\n", desc);
                                    
                        // Remove connection
                        remove_conn(desc, pool);

                        --hold_fd;	// Reduce the max fd;
                        pool->nr_conns -= 1;	// Reduce number of connections
						
						
                        if((pool->nr_conns == 0) || (pool->conn_head) == NULL) 
                        {
							end_server = 1;
							pool->maxfd = socket_listen;	// Restore the max fd which is the listening socket.
						}
                    }
                    
                    else 
                    {
						printf("Descriptor %d is readable.\n", desc);
                    
						printf("%d bytes received from sd: %d.\n", received_data, desc);
						
						conn_t *cons = pool->conn_head;
						
						while(cons != NULL)
						{
							if(cons->fd != desc)
							{
								FD_SET((received_data - 1), &pool->ready_write_set);
								add_msg(cons->fd, recv_buffer, received_data, pool);
								write_to_client(cons->fd, pool);
							}
							cons = cons->next;
						}
					}
                }						                  
			} /* End of if (FD_ISSET()) */	 
      } /* End of loop through selectable descriptors */

   } while (end_server == 0);

	/*************************************************************/
	/* If we are here, Control-C was typed,						 */
	/* clean up all open connections					         */
	/*********************************************************h****/
	for(int open_conns = 0; open_conns <= pool->maxfd; ++open_conns)
	{
		if(FD_ISSET(open_conns, &master))
		{
			CLOSE_SOCKET(open_conns);
		}
	}
	
	CLOSE_SOCKET(socket_listen);
	
	return 0;
}



int init_pool(conn_pool_t* pool) 
{
	//initialized all fields
	pool->maxfd = 0;
	pool->nready = 0;
	
	FD_ZERO(&(pool->read_set));
	FD_ZERO(&(pool->ready_read_set));
	FD_ZERO(&(pool->write_set));
	FD_ZERO(&(pool->ready_write_set));
	
	pool->conn_head = NULL;
	pool->conn_tail = NULL;
	pool->nr_conns = 0;

	return SUCCESS;
}

int add_conn(int sd, conn_pool_t* pool) {
	/*
	 * 1. allocate connection and init fields
	 * 2. add connection to pool
	 * */
	 
	conn_t *new_conn = (conn_t*)malloc(sizeof(conn_t));
	new_conn->fd = sd;
	
	if(pool->conn_head == NULL) pool->conn_tail = pool->conn_head = new_conn;
	
	else
	{
		pool->conn_tail->next = new_conn;
		new_conn->prev = pool->conn_tail;
		pool->conn_tail = new_conn;
	}
	
	return SUCCESS;
}


int remove_conn(int sd, conn_pool_t* pool) {
	/*
	* 1. remove connection from pool 
	* 2. deallocate connection 
	* 3. remove from sets 
	* 4. update max_fd if needed 
	*/

	conn_t *current = pool->conn_head;
	
	char found = 'n';
	
	if(current == NULL) return FAILURE;

	while(current != NULL)
	{
		if(current->fd == sd)
		{
			found = 'y'; 
			break;
		}
		else
			current = current->next;
	}
	
	if(found == 'y')
	{
		FD_CLR(current->fd, &master);
	
		if(current == pool->conn_head)
		{
			pool->conn_head = pool->conn_head->next;
			
			if(pool->conn_head != NULL) 
				pool->conn_head->prev = NULL;
			else 
				pool->conn_tail = NULL;
		
		}
		
		else if(current == pool->conn_tail)
		{
			pool->conn_tail = current->prev;
			pool->conn_tail->next = NULL;		
		}
		else
		{
			current->prev->next = current->next;
			current->next->prev = current->prev;
		}

		free(current);
	}
	
	else 
	{
		perror("Connection not found in pool!\n");
		return FAILURE;
	}
	
	return SUCCESS;
}

int add_msg(int sd,char* buffer,int len,conn_pool_t* pool) {
	
	/*
	 * 1. add msg_t to write queue of all other connections 
	 * 2. set each fd to check if ready to write 
	 */
	 
	conn_t *con = pool->conn_head;
	
	msg_t *new_msg = (msg_t*)malloc(sizeof(msg_t));
	new_msg->message = (char*)malloc(strlen(buffer) + 1);
	strcpy(new_msg->message, buffer);
	new_msg->size = len;
	
	while(con != NULL)
	{

		if(con->write_msg_head == NULL) con->write_msg_head = con->write_msg_tail = new_msg;
		
		else
		{
			con->write_msg_tail->next = new_msg;
			new_msg->prev = con->write_msg_tail;
			con->write_msg_tail = new_msg;
		}
		
		con = con->next;
	}
	
	return SUCCESS;
}

int write_to_client(int sd,conn_pool_t* pool) {
	
	/*
	 * 1. write all msgs in queue 
	 * 2. deallocate each writen msg 
	 * 3. if all msgs were writen successfully, there is nothing else to write to this fd... */
	msg_t *msg_in_buffer = pool->conn_head->write_msg_head;
	
	while(msg_in_buffer)
	{
		send(sd, msg_in_buffer->message, msg_in_buffer->size, 0);
		msg_in_buffer->message = 0;
		msg_in_buffer = msg_in_buffer->next;
	}
	
	return SUCCESS;
}


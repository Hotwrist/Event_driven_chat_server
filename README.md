# Event-driven Chat Server
 
## Description

  This is an event-driven chat server whose function is to forward each incoming message to all clients connected,
  except for the client connection over which the message was received.
  
## Usage

  Command line usage: server <port>
  **<Port>** is the port number the server will listen on. The port should be in the range of 1 to (2^16) i.e 1 to 65536

Requirements
------------

  You need GCC installed to run the server.

Quick start
-----------

    $ gcc chatServer.c  -o server
    $ ./server 8080 

## Options

	8080 - the port the server will listen on.


## How does it work?

   The server runs and uses select() in a loop to check for the socket that is ready for reading or writing.
   If the socket is not the listening socket, it checks to see if the socket is ready for reading. If it is ready,
   it reads from the socket and then forward the message to all other connected socket or client except the one which
   the message was received from.
   
## chatServer.h
   
   This file contains definitions or constants and function declarations.
  
   
## chatServer.c

	This is the server file. It handles socket connection from clients, accepts the client connection and forwards messages.
	
	The functions available in the file are:
	
**init_pool** : This handles the initialization of the connection pool.
**add_conn** : Whenever the server accept new connection, this function is used to add it to the pool of connections
**remove_conn** : This removes a connection from the connection queue incase of termination by peer or the server.
**add_msg** : Whenever the server reads a message from a connection, the message is added to the message queue.
**write_to_client** : This is the function that forwards the message in the message queue to all other clients except
the one where the message came from.

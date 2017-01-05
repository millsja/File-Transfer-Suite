// Filename: ftserver.c
// Author: James Mills | 932704566
// Date: Fall 2016
// Course/Project: CS372/P2
// Description: this program handles incoming requests
// 				for info and transfer relating to files
// 				in its current working directory. more
// 				specifically, it routes three types of 
// 				requests in the following ways:
//
// 				1. invalid command: ftserver instance
// 				responds with an error message on the
// 				same socket
//
// 				2. list request: ftserver, behaving like
// 				a client temporarily, opens a second
// 				connection with the client host and
// 				sends the list of local files that way
//
// 				3. get request: upon receiving a request
// 				for a specific file, the ftserver instance
// 				will again open a second connection with
// 				the client host and send the file over
// 				that connection
//
// 				Please note: I have reused code from my 
// 				submission for CS372 Project 1 wherever
// 				it is applicable.


#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <string.h> // for strtok, strncmp
#include <netdb.h>
#include <unistd.h> // for file checking
#include <dirent.h> // to get list of local files

#define BUFF_SIZE 500
#define BACKLOG 5


// sends message along main connection back to client host
int sendMessage( int sockfd, char* msg ){
	// the buffer that we'll use for pushing and pulling messages
	// from the socket
	char buffer[BUFF_SIZE];
	memset(buffer, 0, BUFF_SIZE);

	// copy our message into the buffer
	int q;
	for( q = 0; msg[q] != '\0' && q < BUFF_SIZE - 1; q++ ){
		buffer[q] = msg[q];
	}
	
	// send our message to the server user
	send(sockfd, buffer, BUFF_SIZE, 0);

	return 0;

}


// takes a filename, opens it up and gets its bytecount.
// returns 0 on error. the fseek method for counting
// bytes in this function comes from stack overflow
// question 238603
unsigned int getFileSize( char* filename ){
	FILE* fp = fopen(filename, "rb");
	fseeko(fp, 0, SEEK_END); // move pointer to eof
	unsigned int size = 0; // to handle larger files
	size = ftello(fp); // get pointer location
	fclose(fp);
	return size;
}


// opens up the requested file and then sends it over
// to the client in 500-byte chunks till all is sent
int sendFileChunks( int sockfd, char* filename ){

	// get the number of bytes in the file, so we can
	// alert the client how much to expect
	unsigned int filesize = getFileSize( filename );

	// if filesize is less than zero, we have a problem
	if( filesize < 1 ) return -1;

	// otherwise, start by sending the filesize over
	char sizeStr[24];
	sprintf(sizeStr, "%u", filesize);
	sendMessage( sockfd, sizeStr );

	char buffer[BUFF_SIZE];
	int eof = -1;
	FILE* fp = fopen(filename, "r");
	while(eof != 0){
		memset(&buffer, 0, BUFF_SIZE);
		eof = fread(&buffer, BUFF_SIZE - 1, sizeof(char), fp);
		sendMessage(sockfd, buffer);
	}
	fclose(fp);

	return 0;			
}


// gets and sends list of files in the current working directory
// this function relies on direction supplied in the stack
// overflow question 4204666
int sendLS( int sockfd ){
	DIR *d;
	struct dirent *dir;
	d = opendir(".");
	while( (dir = readdir(d)) != NULL ){
		sendMessage( sockfd, dir->d_name );

	}
	closedir(d);
	sendMessage( sockfd, "?q" );
	return 0;
}


// initiates our connection to the server. returns
// a connected socket or -1 if the connection failed
int initConnect( char* inputAddr, char* portNo ){
	printf("Connecting to %s at port %s...\n", inputAddr, portNo);

	// c struct for holding host address information
	// zero out the struct and then initialize the address
	// family, the address, and the port with the network-order 
	// port number
	struct addrinfo* servAddr;
	struct addrinfo hints;
	int infoError;

	// configure address info
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;

	// make a list of address structs based on the data we provided
	infoError = getaddrinfo( inputAddr, portNo, &hints, &servAddr );
	if( infoError != 0 ){
		perror("Failed to initialize address struct...\n");
		return -1;
	}

	// the socket file descriptor
	int sockfd = socket(servAddr->ai_family, 
						servAddr->ai_socktype,
						servAddr->ai_protocol);
	if( sockfd < 0 ){
		perror("Couldn't establish socket...\n");
		return -1;
	} 
	
	// connect to the host and error out if you can't
	if( connect(sockfd, servAddr->ai_addr, servAddr->ai_addrlen) < 0 ){
		perror("Couldn't connect to client...\n");
		return -1;
	}
	else{
		// printf("Connected!\n");
	}

	return sockfd;

}


// retrieves the filename from a get command
char* getFilename( char* msg ){
	// create copy of our buffer
	char* msgCpy = malloc( sizeof(msg) );
	memset(msgCpy, 0, sizeof(msg));
	strcpy( msgCpy, msg );

	// set up our new filename string
	char* filename = malloc( sizeof(msg) );
	memset(filename, 0, sizeof(msg));

	// seek out our filename (delimited by space chars)
	// and copy to filename string
	filename = strtok(msgCpy, " ");
	filename = strtok(NULL, " ");

	printf("filename: %s\n", filename);
	return filename;
}


// retrieves the port from a list command
char* getPort( char* msg ){
	// create copy of our buffer
	char* msgCpy = malloc( sizeof(msg) );
	memset(msgCpy, 0, sizeof(msg));
	strcpy( msgCpy, msg );


	// set up our new filename string
	char* port = malloc( sizeof(msg) );
	memset(port, 0, sizeof(msg));

	// seek out our filename (delimited by space chars)
	// and copy to filename string
	port = strtok(msgCpy, " ");
	if( strcmp(port, "LIST") == 0 ){
		port = strtok(NULL, " "); // port
		
	}
	else if( strcmp(port, "GET") == 0 ){
		port = strtok(NULL, " "); // filename
		port = strtok(NULL, " "); // port
		printf("Port %s found...\n", port);
	}

	return port;
}


// checks the filename supplied by the client host to see
// whether such a file actually exists
// credit to stack overflow question 230062 for unistd/access
int checkFile( char* msg ){
	char* filename = getFilename( msg );
	return access(filename, R_OK); 

}


// parses command: if LIST, returns 0; if GET, 
// returns 1; for now, -1 on error
int parseCommand( char* cmd ){
	if( strncmp(cmd, "LIST", 4) == 0 ) return 0; // good list
	else if( strncmp(cmd, "GET", 3) == 0){
		if( checkFile( cmd ) == 0 ){
			return 1; // good get
		}
		else{
			return -2; // bad filename
		}
	}
	else return -1;	// otherwise bad command
}


// initializes new connection with client users and sends
// a list of files local to the server's current working
// directory
int sendList( int sockfd, char* host, char* buffer ){
	char* port = getPort( buffer );
	int newSock = initConnect( host, port );
	sendLS( newSock );
	close(newSock);
	printf("File list sent to client...\n");
	return 0;
}


// initializes new connection with client users and sends
// request file 
int sendFile( int sockfd, char* host, char* buffer ){
	char* port = getPort( buffer );
	char* filename = getFilename( buffer );
	int newSock = initConnect( host, port );
	sendFileChunks( newSock, filename );
	printf("File sent to client...\n");
	return 0;
}


// handles the incoming message from our new client
// returns a number indicating what should happen 
// next, e.g. -1 indicates error
int recvMessage( int sockfd, char* host ){

	// get the next message from the server
	char buffer[BUFF_SIZE];
	memset(buffer, 0, BUFF_SIZE);
	recv(sockfd, buffer, sizeof(buffer), 0);
	printf("Client: %s\n", buffer);	

	int action = parseCommand(buffer);

	char* msg = malloc( BUFF_SIZE );
	memset(msg, 0, BUFF_SIZE);
	switch(action){
		case 0: // good list detected

			// now send our message to the user	
			strcpy( msg, "OKAY: Processing LIST action..." );
			sendMessage( sockfd, msg );
			sleep(1);
			sendList( sockfd, host, buffer );
			
			break;

		case 1: // good get detected

			// now send our message to the user	
			strcpy( msg, "OKAY: Processing GET action..." );
			sendMessage( sockfd, msg );
			sleep(1);
			sendFile( sockfd, host, buffer );
			
			break;

		case -2: // good get, bad file
			
			// now send our message to the user	
			strcpy( msg, "ERROR: either a file does not exist "
						 "with that name or it is protected..." );
			sendMessage( sockfd, msg );
			break;

		default: // otherwise invalid 
			
			// now send our message to the user	
			strcpy( msg, "ERROR: an error has occured processing "
						 "your request. Please try again." );
			sendMessage( sockfd, msg );
			break;
	}


	// otherwise return our action return 1 or 2
	return action;	
}


// connect to a server and send messages back and forth till one
// of the hosts quits
int handleNewClient( int sockfd, char* host ){

	// wait for a response from the server user
	int results = recvMessage( sockfd, host );
	

	return 0;
}


// initializes our server and waits for a connection. This
// is heavily based on Beej's sockets tutorial and uses
// much of the skeleton provided in his examples
int initServer( char* portno ){
	int status; // will hold success/fail info on getaddrinfo
	struct sockaddr_storage clientAddr;
	socklen_t clientAddrLen;
	int sockfd, spawnfd; // fds for initial and accepted sockets
	struct addrinfo hints; // server info for pass to getaddrinfo
	int serverStatus;

	// this will hold a linked list of results from the
	// getaddrinfo call
	struct addrinfo *infoResults; 

	// let's set up our hints...
	memset(&hints, 0, sizeof(hints)); // zero out our hints struct
	hints.ai_family = AF_INET; // not going to worry about IPv6
	hints.ai_socktype = SOCK_STREAM; // TCP connection
	hints.ai_flags = AI_PASSIVE; // use localhost for address

	// get address info i.e. DNS and service name lookups
	// and whatnot
	status = getaddrinfo(NULL, portno, &hints, &infoResults);
	if(status != 0){
		printf("Error: %s\n", gai_strerror(status));
		return -1;
	}
	
	// create socket
	sockfd = socket(infoResults->ai_family, infoResults->ai_socktype,
					infoResults->ai_protocol);
	if( sockfd == -1 ){
		printf("Error: couldn't create socket...\n");
		return -1;
	}
	
	// bind the socket to the given port number
	if( bind(sockfd, infoResults->ai_addr, infoResults->ai_addrlen) == -1){
		close(sockfd);
		printf("Error: couldn't bind to socket...\n");
		return -1;
	}
	
	// clean up our linked list
	// freeaddrinfo(infoResults);

	// start listen for connections
	if( listen(sockfd, BACKLOG) == -1){
		printf("Error: couldn't listen to socket...\n");
		return -1;
	}

	// alert user that server is up and running
	printf("Server open at %s...\n", portno);

	// accept some connections, return a new socket file
	// descriptor to pass off to our client handler			
	while(1){
		// make sure accept doesn't put more bytes into
		// the clientAddr struct than it can handle
		clientAddrLen = sizeof(clientAddr);

		// get our fresh socket file descriptor  for the
		// new connection
		spawnfd = accept(sockfd, (struct sockaddr *)&clientAddr, &clientAddrLen);

		// get our client's host info
		// source: beej's reference on getnameinfo
		char host[1024];
		char service[256];
		getnameinfo( (struct sockaddr *)&clientAddr, 
						sizeof(struct sockaddr), 
						host, sizeof(host),
						service, sizeof(service), 0);
		printf("New client at %s...\n", host);

		// call the function to handle our new connection	
		handleNewClient( spawnfd, host );
	}

}


// checks command-line input for valid number of arguments
// and safe port number range 
int checkValidInput( int argc, char* argv[] ){
	// check for valid input
	if( argc != 2){
		printf("Usage: ftserver port...\n");
		return -1;

	}

	// check for valid port range
	// must be positive and less than max port number
	int port = atoi( argv[1] );
	if( port < 0 || port > 65535 ){
		printf("Error: invalid port number...\n");
		return -1;

	}

	// must be outside of reserved port range 
	else if( port < 1024 ){
		printf("Error: privileged port number...\n");
		return -1;

	}

	return 0;

}


// main function
int main(int argc, char* argv[]){
	// run our input validation
	if(checkValidInput( argc, argv ) < 0) return -1;

	// if all's well, let's set up our server	
	initServer( argv[1] );

	return 0;		

}

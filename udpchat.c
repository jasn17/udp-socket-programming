/**
* Jason Lin 
* Dr. D
* CPSC 3600
* Homework 2
* Please refer to homework instruction on how this program works
*/ 

// Needed this for getaddrinfo() function, ask compiler for functions and constants from the POSTIX.1-2001 spec. 
#define _POSIX_C_SOURCE 200112L
    
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>

#define MAXSTRINGLENGTH 128

// Prints an error message with user context and exits
void DieWithUserMessage(const char *msg, const char *detail) {
  fprintf(stderr, "%s: %s\n", msg, detail);
  exit(1);
}

// Prints a system error message and exits
void DieWithSystemMessage(const char *msg) {
  perror(msg);
  exit(1);
}

// Checks if the input message contains "Goodbye!", return true
bool containsGoodbye(const char *msg) {
  return strstr(msg, "Goodbye!") != NULL;
}

// CLIENT MODE given ./udpchat <server ip> <port>
// Connects to server, sends messages, and receives responses until it sends "Goodbye!"
void runClient(const char *serverIP, const char *port) {
  	struct addrinfo hints, *servAddr;
  	memset(&hints, 0, sizeof(hints) );
  	// Allows for IPv4 or 6
  	hints.ai_family = AF_UNSPEC; 
  	// UDP socket
  	hints.ai_socktype = SOCK_DGRAM;
  	// UDP protocol
  	hints.ai_protocol = IPPROTO_UDP;

  	// Translate servere name/IP and port into address
  	int rtnVal = getaddrinfo( serverIP, port, &hints, &servAddr);
  	if (rtnVal != 0) {
    		DieWithUserMessage("getaddrinfo() failed", gai_strerror(rtnVal));
	}

 	// Create UDP socket
  	int sock = socket(servAddr->ai_family, servAddr->ai_socktype , servAddr->ai_protocol);
  	if (sock  < 0) {
    		DieWithSystemMessage("socket() failed");
	}

  	char buffer[MAXSTRINGLENGTH + 1];
  	struct sockaddr_storage fromAddr;
  	socklen_t fromAddrLen = sizeof(fromAddr);
	
	// Message loop, socket sends a message and wait for response
 	while (true) {
    		printf("user: ");
    		fflush(stdout);

		// Read message
    		if (fgets(buffer, sizeof(buffer), stdin) == NULL) {
      			fprintf(stderr, "[WARN] Could not read from stdin.\n");
      			break;
    		}
	
	
    		size_t msgLen = strlen(buffer);

		// Prevent sending empty messages
    		if (msgLen > 0 && buffer[msgLen - 1] == '\n') buffer[msgLen - 1] = '\0';
    		if (strlen(buffer) == 0) {
      			fprintf(stderr, "[INFO] Empty message not sent. Try again.\n");
      			continue;
    		}

    		// Send the message to server
    		ssize_t numBytes = sendto(sock, buffer, strlen(buffer), 0,
        		servAddr->ai_addr, servAddr->ai_addrlen);
    		if (numBytes < 0) {
      			perror("sendto() failed");
      			continue; // Don't exit on one bad send
    		}

    		// Receive the server's response
    		numBytes = recvfrom(sock, buffer, MAXSTRINGLENGTH, 0,
    		(struct sockaddr *)&fromAddr, &fromAddrLen);
    		if (numBytes < 0) {
      			perror("recvfrom() failed");
      			continue;
    		}

    		buffer[numBytes] = '\0';
    		printf("server: %s\n", buffer);

    		if (containsGoodbye(buffer))  {
			break; // End if we get a goodbye
		}
	}

  	freeaddrinfo(servAddr);
  	close(sock);
}

// SERVER MODE given ./udpchat <port>
// Waits for a client to send messages, replies after each, and prints conversation
void runServer(const char *port) {
  	int sock;
  	struct addrinfo hints, *servAddr;
	// Same as client
  	memset(&hints, 0, sizeof(hints));
  	hints.ai_family = AF_UNSPEC;
  	hints.ai_socktype = SOCK_DGRAM;
  	hints.ai_flags = AI_PASSIVE;
  	hints.ai_protocol = IPPROTO_UDP;
	
	// Bind to address struct
  	int rtnVal = getaddrinfo(NULL, port, &hints, &servAddr);
  	if (rtnVal != 0 ) {
    		DieWithUserMessage( "getaddrinfo() failed", gai_strerror(rtnVal));
	}
	
	// Create socket
  	sock = socket(servAddr->ai_family, servAddr->ai_socktype, servAddr->ai_protocol);
  	if (sock < 0) {
    		DieWithSystemMessage("socket() failed");
  	}
	
	// Bind socket to address
  	if (bind(sock, servAddr->ai_addr, servAddr->ai_addrlen) < 0) {
    		DieWithSystemMessage("bind() failed");
	}

  	freeaddrinfo(servAddr);

  	char buffer[MAXSTRINGLENGTH + 1];
	// Store client address
  	struct sockaddr_storage clientAddr;
  	socklen_t  clientAddrLen = sizeof(clientAddr);
	// Print string client address
  	char clientIP[INET6_ADDRSTRLEN];

  	while (true) {
    		printf("Waiting for a client...\n");

    		ssize_t numBytes = recvfrom(sock, buffer, MAXSTRINGLENGTH, 0,
        	(struct sockaddr *)&clientAddr, &clientAddrLen);
    		if (numBytes < 0) {
      			perror("recvfrom() failed");
      			continue;
    		}
		
		// Convert client address to string
    		inet_ntop(clientAddr.ss_family,
      		clientAddr.ss_family == AF_INET ?
      		(void *)&((struct sockaddr_in *)&clientAddr)->sin_addr :
      		(void *)&((struct sockaddr_in6 *)&clientAddr)->sin6_addr,
     		clientIP, sizeof(clientIP));

    		printf("Talking to client %s\n", clientIP);

    		while (true) {
      			buffer[numBytes] = '\0';
      			printf("user: %s\n", buffer);
			
			// Detects Goodbye! and end session
      			if (containsGoodbye(buffer)) {
        			const char *bye = "Goodbye!";
        			sendto(sock, bye, strlen(bye), 0,
               			(struct sockaddr *)&clientAddr, clientAddrLen);
        			printf("server: Goodbye!\n");
        			printf( "Conversation with %s ended\n", clientIP);
        			break;
     	 		}

     			printf("server: ");
      			fflush(stdout);
			if (fgets(buffer, sizeof(buffer), stdin) == NULL) {
        			fprintf(stderr, "[WARN] Could not read server input.\n");
        			break;
     			}

      			size_t msgLen = strlen(buffer);
      			if (msgLen > 0 && buffer[msgLen - 1] == '\n') buffer[msgLen - 1] = '\0';
      			if (strlen(buffer) == 0) {
        			fprintf(stderr, "[INFO] Skipping empty response.\n");
        			continue;
      			}
	
			// Respond to client
      			sendto(sock, buffer, strlen(buffer), 0, (struct sockaddr *)&clientAddr, clientAddrLen);

      			numBytes = recvfrom(sock, buffer, MAXSTRINGLENGTH, 0, (struct sockaddr *)&clientAddr, &clientAddrLen);
      			if (numBytes < 0) {
        			perror("recvfrom() failed");
        			break;
      			}
    		}
  	}
  	close(sock);
}

int main(int argc, char *argv[]) {
 	if (argc == 2) {
    		runServer(argv[1]); // Server mode: just port
  	} 
	else if (argc == 3 ) {
    		runClient(argv[1], argv[2]); // Client mode: server IP and port
  	} 
	else {
    		DieWithUserMessage("Usage", "udpchat <port>    OR    udpchat <serverIP> <port>");
  	}
  	return 0;
}

#include <sys/socket.h>
#include <sys/types.h>
#include <stdio.h>
#include <netinet/in.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>


// create connction
int create_connection(char* addr, int port) {
	int sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	struct sockaddr_in server;
	memset(&server, 0, sizeof(server));
	server.sin_family = AF_INET;
	// server.sin_addr.s_addr = inet_addr(addr);
	server.sin_port = htons(port);
	if(inet_pton(AF_INET, addr, &server.sin_addr) == -1) {
		close(sockfd);
		exit(EXIT_FAILURE);
	}

	int b = bind(sockfd, (struct sockaddr*)&server, sizeof(server));
	if(b == -1) {
		close(sockfd);
		exit(EXIT_FAILURE);
	}
	int l = listen(sockfd, 1);
	if(l == -1) {
		close(sockfd);
		exit(EXIT_FAILURE);
	}
	return sockfd;
}

// Accept incoming connections
int client_connect(int socket_id) {
	struct sockaddr_in client_addr;
	socklen_t clilen = sizeof(client_addr);
	int newsockfd = accept(socket_id, (struct sockaddr*)&client_addr, &clilen);
	if(newsockfd == -1) {
		close(newsockfd);
		exit(EXIT_FAILURE);
	}
	return newsockfd;
}

// Echo input from client
void echo_input(int socket_id) {
	char buffer[256];
	char buffer1[256];
	while(1) {
		int bytes;
		char *tok;
		char message[256];
		char echo[256];
		char reply1[256];
		bzero(buffer, 256);
		bzero(reply1, 256);
		int r = recv(socket_id, buffer, 256, 0);
		strcpy(buffer1,buffer);
		if(r == -1) {
			close(socket_id);
			break;
		}
		int col=0;
		for(int i=0; i<strlen(buffer); i++) {
			if(buffer[i]==':') {
				col++;
			}
		}
		tok = strtok(buffer, ":");
		if(strcmp(tok, "ECHO") != 0) {
			sprintf(reply1, "Invalid command: Unable to echo!");
			int w = send(socket_id, reply1, strlen(reply1), 0);
			if(w == -1) {
				close(socket_id);
				break;
			}
		}
		else {
			if(col==1) {
				sscanf(buffer1, "%255[^:]:%s", echo, message);
				strcpy(reply1, "OHCE:");
				strncat(reply1, message, 5);
				int w = send(socket_id, reply1, strlen(reply1), 0);
				if(w == -1) {
					close(socket_id);
					break;
				}
			}
			else {
				sscanf(buffer1, "%255[^:]:%255[^:]:%d", echo, message, &bytes);
				if(bytes < 0) {
					strncpy(reply1, "Error: Negative number of bytes", 31);
					int w = send(socket_id, reply1, 31, 0);
					if(w == -1) {
						close(socket_id);
						break;
					}
				}
				else if(strlen(message) < bytes) {
					sprintf(reply1, "OHCE:%s (%d bytes sent)", message, strlen(message));
					int w = send(socket_id, reply1, strlen(reply1), 0);
					if(w == -1) {
						close(socket_id);
						break;
					}
				}
				else{
					strncpy(reply1, "OHCE:", 5);
					strncat(reply1,message,bytes);
					int w = send(socket_id, reply1, strlen(reply1), 0);
					if(w == -1) {
						close(socket_id);
						break;
					}
				}
			}
		}
	}
}

int main(int argc, char *argv[])
{
    if (argc != 3)
	{
		printf("Use 2 cli arguments\n");
		return -1;
	}
	
	// extract the address and port from the command line arguments
	char addr[INET_ADDRSTRLEN];
	strcpy(addr, argv[1]);
	int port = atoi(argv[2]);
	int socket_id = create_connection(addr, port);
    int client_id = client_connect(socket_id);
	echo_input(client_id);
    close(socket_id);
    return 0;    
}

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
		close(socket_id);
		close(newsockfd);
		exit(EXIT_FAILURE);
	}
	return newsockfd;
}

// Echo input from client
void echo_input(int socket_id) {
	char buffer[256];
	while(1) {
		bzero(buffer, 256);
		int r = recv(socket_id, buffer, 256, 0);
		if(r == -1) {
			close(socket_id);
			break;
		}
		if(strlen(buffer) < 5) {
			strncpy(buffer, "Error: Message length must be more than 5 characters", 52);
			int w = send(socket_id, buffer, 52, 0);
			if(w == -1) {
				close(socket_id);
				break;
			}
		}
		else {
			printf("%s",buffer);
			fflush(stdout);
			int w = send(socket_id, buffer, strlen(buffer), 0);
			if(w == -1) {
				close(socket_id);
				break;
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

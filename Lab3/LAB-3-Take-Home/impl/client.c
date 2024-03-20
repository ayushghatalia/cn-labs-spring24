#include <sys/socket.h>
#include <sys/types.h>
#include <stdio.h>
#include <netinet/in.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>


// Create a connection to the server
int create_connection(char* addr, int port) {
	int sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if(sockfd == -1) {
		exit(EXIT_FAILURE);
	}

	struct sockaddr_in server;
	memset(&server, 0, sizeof(server));
	server.sin_family = AF_INET;
	// server.sin_addr.s_addr = inet_addr(addr);
	server.sin_port = htons(port);

	if(inet_pton(AF_INET, addr, &server.sin_addr) <= 0){
		close(sockfd);
		exit(1);
	}

	int c = connect(sockfd, (struct sockaddr*)&server, sizeof(server));
	if(c == -1) {
		printf("Could not find server");
		fflush(stdout);
		close(sockfd);
		exit(EXIT_FAILURE);
	}
	return sockfd;
}

void send_data(int socket_id) {
	char buffer[256];
	bzero(buffer, 256);
	fgets(buffer, 256, stdin);
	if(!strncmp("EXIT", buffer, 4)) {
		printf("Client exited successfully");
		fflush(stdout);
		close(socket_id);
		exit(EXIT_SUCCESS);
	}
	int w = send(socket_id, buffer, strlen(buffer), 0);
	if(w == -1) {
		close(socket_id);
		exit(EXIT_FAILURE);
	}
}

// Receive input from the server
void recv_data(int socket_id) {
	char buffer[256];
	bzero(buffer, 256);
	int r = recv(socket_id, buffer, 256, 0);
	if(r == -1) {
		close(socket_id);
		exit(EXIT_FAILURE);
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

	while (1)
    {
        send_data(socket_id);
        recv_data(socket_id);
    }
}
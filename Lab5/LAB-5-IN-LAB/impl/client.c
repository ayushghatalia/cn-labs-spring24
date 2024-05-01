#include <sys/socket.h>
#include <sys/types.h>
#include <stdio.h>
#include <netinet/in.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>

char CLIENT_NAME[1024];

// Process the reply from the server
void processReply(char *reply, int recv_count, int socket_id){

    if(reply[strlen(reply) - 1] == '\n'){
        printf("%s", reply);  
    } 
    else{
        printf("%s\n", reply);
    }
    fflush(stdout);
}

// For the thread that listens only to the server and prints to STDOUT
void* listenToServer(void* args){

    // Dereference an get socket_id
    int socket_id = *(int*)args;

    while(1){
        // Get reply from server
        char reply[1024];
        int recv_count;
        memset(reply, 0, 1024); // reset reply to all 0s
        if((recv_count = recv(socket_id, reply, 1024, 0)) == -1){
            close(socket_id);
            int* ret_status = malloc(sizeof(int));
            *ret_status = 1;
            pthread_exit(ret_status);
        }

        // If server has closed the connection, then exit
        if(recv_count == 0){
            int* ret_status = malloc(sizeof(int));
            *ret_status = 0;
            pthread_exit(ret_status);
        }

        // Take appropriate action
        processReply(reply, recv_count, socket_id);
    }
}

// For the thread that listens only to stdin
void* listenToSTDIN(void* args){

    int socket_id = *(int*)args;
    int *ret_status = malloc(sizeof(int));
    while(1){
        char msg[1024];
        memset(msg, 0, 1024); // reset message to all 0s
        fflush(stdin);
        if(fgets(msg, 1024, stdin) != NULL){ // Error Check to prevent problematic input
            if(strcmp(msg, "EXIT\n") == 0){ // EXIT check
                if (send(socket_id, msg, 1024, 0) == -1){
                    *ret_status = 1;
                    pthread_exit(ret_status);
                }
                else{
                    printf("CLIENT TERMINATED: EXITING......\n");
                    fflush(stdout);
                    *ret_status = 0;
                    pthread_exit(ret_status);
                }
            }
            else if(send(socket_id, msg, 1024, 0) == -1){ // Actual send + error check, strlen(msg), 1024
                *ret_status = 1;
                pthread_exit(ret_status);
            }
        }
        else{
            *ret_status = 1;
            pthread_exit(ret_status);
        }
    }
}

// Create a connection to the server
int create_connection(char* addr, int port){

	// Create a TCP Socket on the client
	int client_sockfd;
	if((client_sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1){
        exit(1);
    }

	// Find server
	struct sockaddr_in server_addrinfo;
	server_addrinfo.sin_family = AF_INET;
	server_addrinfo.sin_port = htons(port);
	if(inet_pton(AF_INET, addr, &server_addrinfo.sin_addr) <= 0){
		close(client_sockfd);
		exit(1);
	}

	// Connect
	if(connect(client_sockfd, (struct sockaddr*) &server_addrinfo, sizeof(server_addrinfo)) == -1){
		printf("Could not find server"); // weird to be here
		close(client_sockfd);
		fflush(stdout);
		exit(1);
	}

	// send the client name in response
    memset(CLIENT_NAME, 0, 1024); // reset name to all 0s
    fgets(CLIENT_NAME, 1024, stdin);
    char* name;
    char* type;
    // if(sscanf(CLIENT_NAME,"%s[^|]|%c[^\n]",name,&type) == 3) {
    char tmp[1024];
    strcpy(tmp,CLIENT_NAME);
    name = strtok(tmp,"|");
    type = strtok(NULL,"|");
    // sscanf(CLIENT_NAME,"%s[^|]|%c",name,&type);
    if(CLIENT_NAME[strlen(CLIENT_NAME) - 1] == '\n'){
        CLIENT_NAME[strlen(CLIENT_NAME) - 1] = '\0';
    } // remove the newline character from the end of the string
    char msg_to_send[1024];
    memset(msg_to_send, 0, 1024); // reset message to all 0s
    strcat(msg_to_send, CLIENT_NAME);
    strcat(msg_to_send, "\n");
    if(send(client_sockfd, msg_to_send, 1024, 0) == -1){
        printf("COULD NOT SEND NAME\n"); fflush(stdout);
        close(client_sockfd);
        exit(1);
    }
    if(strncmp(type,"r",1) == 0) {
        char password[1024];
        memset(password,0,1024);
        fgets(password,1024,stdin);
        if(password[strlen(password) - 1] == '\n'){
            password[strlen(password) - 1] = '\0';
        } 
        memset(msg_to_send, 0, 1024); // reset message to all 0s
        strcat(msg_to_send, password);
        // strcat(msg_to_send, "\n");
        if(send(client_sockfd, password, 1024, 0) == -1){
            // printf("COULD NOT SEND NAME\n"); fflush(stdout);
            close(client_sockfd);
            exit(1);
        }
        memset(password,0,1024);
        if(recv(client_sockfd, password, 1024, 0) == -1){
            // printf("COULD NOT SEND NAME\n"); fflush(stdout);
            close(client_sockfd);
            exit(1);
        }
        printf("%s",password);
        fflush(stdout);
    }
    // }

	// Return if everything works out
	return client_sockfd;
}

int main(int argc, char *argv[]){

    if (argc != 3)
	{
		printf("Use 2 cli arguments\n"); // ip and port
		return -1;
	}
    
	// extract the address and port from the command line arguments
	char addr[INET6_ADDRSTRLEN];
	strcpy(addr, argv[1]);
	int port = atoi(argv[2]);
	int socket_id = create_connection(addr, port);

    pthread_t server, stdin_;
    pthread_attr_t server_attr, stdin_attr;
    pthread_attr_init(&server_attr);
    pthread_attr_init(&stdin_attr);

    int* arg = malloc(sizeof(int));
    *arg = socket_id;
    pthread_create(&server, &server_attr, listenToServer, arg);
    pthread_create(&stdin_, &stdin_attr, listenToSTDIN, arg);

	pthread_join(server, NULL);
    void *_ret_status;
    pthread_join(stdin_, &_ret_status);
    int ret_status = *((int*) _ret_status);
    if(ret_status == 1){
        printf("Error has occured\n");
        fflush(stdout);
    }
    close(socket_id);
    return 0;
}

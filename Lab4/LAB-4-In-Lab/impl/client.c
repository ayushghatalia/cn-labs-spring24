#include <sys/socket.h>
#include <sys/types.h>
#include <stdio.h>
#include <netinet/in.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

char username[1024];
char mode[1024];

int list_checker=0;

// Create a connection to the server
int create_connection(char* addr, int port) {
	int client_sockfd;
	if ((client_sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
	{
		exit(1);
	}
	struct sockaddr_in server_addrinfo;
	server_addrinfo.sin_family = AF_INET;
	server_addrinfo.sin_port = htons(port);
	if (inet_pton(AF_INET, addr, &server_addrinfo.sin_addr) <= 0)
	{
		close(client_sockfd);
		exit(1);
	}
	if (connect(client_sockfd, (struct sockaddr *)&server_addrinfo, sizeof(server_addrinfo)) == -1)
	{
		printf("Could not find server");
        fflush(stdout);
		close(client_sockfd);
		exit(1);
	}
	return client_sockfd;
}

// Receive input from the server
void recv_data(int socket_id) {
	char reply[1024];
    char msg[1024];
	memset(reply, 0, 1024);
    memset(msg, 0, 1024);
	if (list_checker!=1&&recv(socket_id, reply, 1024, 0) == -1){
		exit(1);
	}
	if(list_checker!=1&&strncmp(reply,"NAME",4)==0){
        printf("INITIALIZING......\n");
        fflush(stdout);
        if (send(socket_id, username, (int)strlen(username), 0) == -1){
            close(socket_id);
            exit(1);
        }
    }
    else if(list_checker!=1&&strncmp(reply,"MODE",4)==0){
        // printf("INITIALIZING......\n");
        // fflush(stdout);
        if (send(socket_id, mode, (int)strlen(mode), 0) == -1){
            close(socket_id);
            exit(1);
        }
    }
    else if(list_checker==1||strncmp(reply,"POLL",4)==0){
        list_checker=0;
        printf("ENTER CMD: ");
        fflush(stdout);
        fgets(msg,1024,stdin);
        // printf("\n");
        // fflush(stdout);
        // scanf("%[^\n]%*c", msg);
        if (send(socket_id, msg, (int)strlen(msg), 0) == -1){
            close(socket_id);
            exit(1);
        }
        char* token_ms=strtok(msg,":");
        if(token_ms!=NULL&&strncmp(token_ms,"MSGC",4)==0){
            if (recv(socket_id, reply, 1024, 0) == -1){
                exit(1);
            }
            token_ms=strtok(NULL,"|");
            char* messge=strtok(NULL,"|");
            char* glob_no=strtok(NULL,"\n");
            printf("%s:%s:%s\n",token_ms,glob_no,messge);
            fflush(stdout);
        }
        else if(strcmp(msg,"EXIT\n")==0){
            printf("CLIENT TERMINATED: EXITING......\n");
            fflush(stdout);
            close(socket_id);
		    exit(1);
        }
        else if(strcmp(msg,"LIST\n")==0){
            if (recv(socket_id, reply, 1024, 0) == -1){
                exit(1);
            }
            char* first=strtok(reply,"\n");
            char* second=strtok(NULL,"\n");
            strncpy(reply,first,strlen(first));
            // reply[strlen(reply)-1]='\0';
            int curr=1;
            char* token=strtok(reply,":");
            while(token!=NULL){
                printf("%d. ",curr);
                printf("%s\n",token);
                fflush(stdout);
                token=strtok(NULL,":");
                curr++;
            }
            if(second!=NULL){
                list_checker=1;
            }
        }
    }
}

int main(int argc, char *argv[])
{
    if (argc != 5)
	{
		printf("Use 4 cli arguments\n");
        fflush(stdout);
		return -1;
	}
    
	// extract the address and port from the command line arguments
	char serverIP[INET6_ADDRSTRLEN];
	unsigned int server_port;
	strcpy(serverIP, argv[1]);
	server_port = atoi(argv[2]);
    memset(username,0,sizeof(username));
    strcpy(username,argv[3]);
    strcat(username,"\n");
    memset(mode,0,sizeof(mode));
    strcpy(mode,argv[4]);
    strcat(mode,"\n"); //err: extra \n?
	int socket_id = create_connection(serverIP, server_port);
	while (1)
    {
        recv_data(socket_id);
    }
    return 0;
}

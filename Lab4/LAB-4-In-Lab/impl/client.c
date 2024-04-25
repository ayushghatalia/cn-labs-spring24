#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

char buffer[1024];
int connection(char* ip, int port, char* username, char* mode) {
    int sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if(sockfd == -1) {
        exit(EXIT_FAILURE);
    }
    struct sockaddr_in server_info;
    if(inet_pton(AF_INET, ip, &server_info.sin_addr) == -1) {
        close(sockfd);
        exit(EXIT_FAILURE);
    }
    server_info.sin_port = htons(port);
    server_info.sin_family = AF_INET;
    if(connect(sockfd, (struct sockaddr*)&server_info, sizeof(server_info)) == -1) {
        close(sockfd);
        exit(EXIT_FAILURE);
    }
    memset(&buffer,0,1024);
    int r = recv(sockfd,buffer,1024,0);
    if(strncmp(buffer,"NAME\n",5) == 0) {
        printf("INITIALIZING......\n");
        fflush(stdout);
        memset(buffer,0,1024);
        strncpy(buffer,username,strlen(username));
        strcat(buffer,"\n");
        int s = send(sockfd, buffer, strlen(buffer),0);
    }
    memset(&buffer,0,1024);
    r = recv(sockfd,buffer,1024,0);
    if(strncmp(buffer,"MODE\n",5) == 0) {
        memset(buffer,0,1024);
        strncpy(buffer,mode,strlen(mode));
        int s = send(sockfd, buffer, strlen(buffer),0);
    }
    return sockfd;
}

void polling(int socket) {
    char msg[1024];
    while(1) {
        memset(buffer,0,1024);
        memset(msg,0,1024);
        
        int r = recv(socket,buffer,1024,0);
        printf("%s\n",buffer);
        fflush(stdout);
        if(strncmp(buffer,"POLL\n",5) == 0) {
            memset(buffer,0,1024);
            printf("ENTER CMD: ");
            fflush(stdout);
            fgets(msg,1024,stdin);
            int s = send(socket,msg,strlen(msg),0);
            if(strncmp(msg,"EXIT\n",5) == 0) {
                printf("CLIENT TERMINATED: EXITING......\n");
                fflush(stdout);
                close(socket);
                exit(EXIT_SUCCESS);
            }
            else if(strcmp(msg,"LIST\n") == 0) {
                memset(buffer,0,1024);
                memset(msg,0,1024);
                int r = recv(socket,buffer,1024,0);
                int bullet=1;
                char* token=strtok(buffer,":");
                while(token!=NULL)
                {
                    printf("%d. %s", bullet,token);
                    bullet++;
                    token=strtok(NULL, ":");
                    if(token!=NULL) {
                        printf("\n");
                    }
                }
            }
            else {
                continue;
            }
        }
        else if(strncmp(buffer,"MSGC:",5) == 0) {    
            memset(msg,0,1024);
            char mesg[256];
            char src[256];
            char message[256];
            int global;
            sscanf(buffer, "%255[^:]:%255[^|]|%255[^|]|%d[^\n]", mesg, src, message, &global);
            printf("%s:%d:%s\n", src, global, message);
            fflush(stdout);
        }
        else {
            break;
        }
    }
}


int main(int argc, char* argv[]) {
    if(argc != 5) {
        exit(EXIT_FAILURE);
    }
    char* ip = argv[1];
    int port = atoi(argv[2]);
    char* username = argv[3];
    char* mode = argv[4];
    int clisock = connection(ip, port, username, mode);
    polling(clisock);
    return 0;
}
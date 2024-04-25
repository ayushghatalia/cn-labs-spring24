#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdarg.h>

int clisockfd[3] = {0};
char username[3][1024];
char buffer[1024];
char* pollmsg = "POLL\n";

int connection(char* ip, int port) {
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
    int b = bind(sockfd,(struct sockaddr*)&server_info, sizeof(server_info));
    if(b == -1) {
        close(sockfd);
        exit(EXIT_FAILURE);
    }
    if(listen(sockfd,3) == -1) {
        close(sockfd);
        exit(EXIT_FAILURE);
    }
    return sockfd;
}

void client_connect(int sockfd) {
    struct sockaddr_in client_info[3];
    socklen_t clilen = sizeof(client_info[0]);
    for(int i=0; i<3; i++) {
        clisockfd[i] = accept(sockfd, (struct sockaddr*)&client_info[i], &clilen);
        if(clisockfd[i] == -1) {
            close(sockfd);
            exit(EXIT_FAILURE);
        }
        memset(username[i],0,1024);
        int s = send(clisockfd[i],"NAME\n",5,0);
        if(s == -1) {
            exit(EXIT_FAILURE);
        }
        int r = recv(clisockfd[i],username[i],1024,0);
        int l = strlen(username[i]);
        username[i][l-1] = '\0';
        if(r == -1) {
            exit(EXIT_FAILURE);
        }
    }
}

// void usernames() {
//     for(int i=0; i<3; i++) {
//         memset(username[i],0,1024);
//         int s = send(clisockfd[i],"NAME\n",5,0);
//         if(s == -1) {
//             exit(EXIT_FAILURE);
//         }
//         int r = recv(clisockfd[i],username[i],1024,0);
//         int l = strlen(username[i]);
//         username[i][l-1] = '\0';
//         if(r == -1) {
//             exit(EXIT_FAILURE);
//         }
//     }
// }

void polling() {
    int exited = 0;
    int i = 0;
    while(exited < 3) {
        int i1 = 0;
        memset(buffer,0,1024);
        if(clisockfd[i] == 0) {
            if(i == 2) {
                if(clisockfd[0] == 0) {
                    i = 1; 
                }
                else {
                    i = 0;
                }
            }
            else {
                i++;
                if(clisockfd[i] == 0) {
                    if(i == 2) {
                        i = 0;
                    }
                    else {
                        i++;
                    }
                }
            }
        }
        int s = send(clisockfd[i],pollmsg,strlen(pollmsg),0);
        int r = recv(clisockfd[i],buffer,1024,0);
        if(strncmp(buffer,"NOOP\n",5) == 0) {
        }
        else if(strncmp(buffer,"EXIT\n",5) == 0) {
            close(clisockfd[i]);
            clisockfd[i] = 0;
            exited++;
        }
        else if(strncmp(buffer,"MESG:",5) == 0) {
            char mesg[1024];
            char message[1024];
            sscanf(buffer,"%255[^:]:%255[^\n]",mesg,message);
            printf("%s:%s\n",username[i],message);
            fflush(stdout);
        }
        else if(strncmp(buffer,"LIST\n",5) == 0) {
            memset(buffer,0,1024);
            for(int j=0; j<3; j++) {
                if(clisockfd[j] != 0) {
                    strncat(buffer,username[j],strlen(username[j]));
                    strcat(buffer,":");
                }
            }
            int len = strlen(buffer);
            buffer[len-1] = '\n';
            int s = send(clisockfd[i],buffer,strlen(buffer),0);
            memset(buffer,0,1024);
        }
        else {
            printf("INVALID CMD\n");
            fflush(stdout);
        }
        i = (i+1)%3;
    }
    printf("SERVER TERMINATED: EXITING......\n");
    fflush(stdout);
}

int main(int argc, char* argv[]) {
    if(argc != 3) {
        exit(EXIT_FAILURE);
    }
    char* ip = argv[1];
    int port = atoi(argv[2]);
    int mainsock = connection(ip, port);
    client_connect(mainsock);
    // usernames();
    polling();
    return 0;
}

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdarg.h>
#include <stdbool.h>

char* pollmsg = "POLL\n";
int active = 0;
int passive = 0;
int global = 0;
FILE *file;

int connection(char* ip, int port, int clients) {
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
    int l = listen(sockfd, clients);
    if(l == -1) {
        close(sockfd);
        exit(EXIT_FAILURE);
    }
    return sockfd;
}

void client_connect(int sockfd, int clients) {
    int clisockfd[clients];
    char mode[clients][1024];
    char username[clients][1024];
    char buffer[1024];
    struct sockaddr_in client_info[clients];
    socklen_t clilen = sizeof(client_info[0]);
    for(int i=0; i<clients; i++) {
        clisockfd[i] = accept(sockfd, (struct sockaddr*)&client_info[i], &clilen);
        if(clisockfd[i] == -1) {
            close(sockfd);
            exit(EXIT_FAILURE);
        }
        memset(username[i],0,1024);
        memset(mode[i],0,1024);
        int s = send(clisockfd[i],"NAME\n",5,0);
        if(s == -1) {
            exit(EXIT_FAILURE);
        }
        int r = recv(clisockfd[i],username[i],1024,0);
        if(r == -1) {
            exit(EXIT_FAILURE);
        }
        int l = strlen(username[i]);
        username[i][l-1] = '\0';
        s = send(clisockfd[i],"MODE\n",5,0);
        if(s == -1) {
            exit(EXIT_FAILURE);
        }
        r = recv(clisockfd[i],mode[i],1024,0);
        if(r == -1) {
            exit(EXIT_FAILURE);
        }
        if(strncmp(mode[i],"active",6) == 0) {
            active++;
        }
        else {
            passive++;
        }
        fprintf(file, "CLIENT: %s; MODE: %s", username[i], mode[i]);
        fflush(stdout);
    }
    int clisockfda[active];
    int clisockfdb[passive];
    char usernamea[active][1024];
    char usernameb[passive][1024];
    int a = 0;
    int p = 0;
    for(int i=0; i<clients; i++) {
        if(strncmp(mode[i],"active",6) == 0) {
            clisockfda[a] = clisockfd[i];
            strcpy(usernamea[a],username[i]);
            a++;
        }
        else {
            clisockfdb[p] = clisockfd[i];
            strcpy(usernameb[p],username[i]);
            p++;
        }
    }
    int exited = 0;
    int i = 0;
    while(exited < active) {
        memset(&buffer,0,1024);
        while(clisockfda[i] == 0) {
            i = (i+1)%active;
        }
        int s = send(clisockfda[i],pollmsg,1024,0);
        int r = recv(clisockfda[i],buffer,1024,0);
        if(strncmp(buffer,"NOOP\n",5) == 0) {
            fprintf(file, "%s: NOOP\n", usernamea[i]);
            fflush(stdout);
        }
        else if(strncmp(buffer,"EXIT\n",5) == 0) {
            fprintf(file, "%s: EXIT\n", usernamea[i]);
            fflush(stdout);
            close(clisockfda[i]);
            clisockfda[i] = 0;
            exited++;
        }
        else if(strncmp(buffer,"MSGC:",5) == 0) {
            bool flag = false;
            global++;
            char mesg[256];
            char dest[256];
            char message[256];
            sscanf(buffer,"%255[^:]:%255[^|]|%255[^\n]",mesg, dest, message);
            memset(buffer,0,1024);
            sprintf(buffer, "MSGC:%s|%s|%d\n",usernamea[i],message,global);
            for(int j=0; j<active; j++) {
                if(strcmp(usernamea[j],dest) == 0 && !clisockfda[j]) {
                    fprintf(file, "INVALID RECIPIENT: %s\n",dest);
                    fflush(stdout);
                    flag = true;
                }
            }
            for(int j=0; j<clients && !flag; j++) {
                if(strcmp(username[j],dest) == 0) {
                    flag = true;
                    fprintf(file, "%s->%s:0%X:%s\n",usernamea[i],dest,global,message);
                    fflush(stdout);
                    int s = send(clisockfd[j], buffer, strlen(buffer),0);
                }
            }
            if(!flag) {
                fprintf(file, "INVALID RECIPIENT: %s\n",dest);
                fflush(stdout);
            }
        }
        else if(strncmp(buffer,"MESG:",5) == 0) {
            char mesg[1024];
            char message[1024];
            sscanf(buffer,"%255[^:]:%255[^\n]",mesg,message);
            printf("%s:%s\n",usernamea[i],message);
            fflush(stdout);
            fprintf(file, "%s: %s\n",usernamea[i],message);
            fflush(stdout);
        }
        else if(strncmp(buffer,"LIST\n",5) == 0) {
            fprintf(file, "%s: LIST\n", usernamea[i]);
            fflush(stdout);
            memset(buffer,0,1024);
            for(int j=0; j<active; j++) {
                if(clisockfda[j] != 0) {
                    strncat(buffer,usernamea[j],strlen(usernamea[j]));
                    strcat(buffer,":");
                }
            }
            for(int j=0; j<passive; j++) {
                strncat(buffer,usernameb[j],strlen(usernameb[j]));
                strcat(buffer,":");
            }
            int len = strlen(buffer);
            buffer[len-1] = '\n';
            int s = send(clisockfda[i],buffer,strlen(buffer),0);
            fprintf(file, "%s", buffer);
            fflush(stdout);
            memset(buffer,0,1024);
        }
        else {
            fprintf(file, "%s: INVALID CMD\n",usernamea[i]);
            fflush(stdout);
            printf("INVALID CMD\n");
            fflush(stdout);
        }
        i = (i+1)%active;
    }
    exited = 0;
    i = 0;
    while(exited < passive) {
        close(clisockfdb[i]);
        i++;
        exited++;
    }
    fprintf(file, "SERVER TERMINATED: EXITING......\n");
    fflush(stdout);
    printf("SERVER TERMINATED: EXITING......\n");
    fflush(stdout);
}

int main(int argc, char* argv[]) {
    if(argc != 4) {
        exit(EXIT_FAILURE);
    }
    char* ip = argv[1];
    int port = atoi(argv[2]);
    int clients = atoi(argv[3]);
    file = fopen("log.txt","w+");
    int mainsock = connection(ip, port, clients);
    client_connect(mainsock, clients);
    return 0;
}
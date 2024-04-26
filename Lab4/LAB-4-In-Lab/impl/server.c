#include <sys/socket.h>
#include <sys/types.h>
#include <stdio.h>
#include <netinet/in.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

char *NAME_MSG = "NAME\n", *POLL_MeSG = "POLL\n", *EXIT_MSG = "EXIT\n";
char *NOOP_MSG = "NOOP\n", *MESG_MSG = "MESG:", *LIST_MSG = "LIST\n";
char *MODE_MSG = "MODE\n", *MSGC_MSG = "MSGC:";

int GLOBAL_IDX = 1;
FILE *logger;


void processReply(char* reply, int *cur_client, int *online_clients, int passive_clients, int *ONLINE_SOCKETS, char **ONLINE_NAMES, int *PASSIVE_SOCKETS, char **PASSIVE_NAMES){

	// Case for NOOP, format: NOOP\n and to be ignored
	if(strcmp(reply, NOOP_MSG) == 0){
		// Log and just return if NOOP
        fprintf(logger, "%s: NOOP\n", ONLINE_NAMES[*cur_client]);
		return;
	}

	// Case for EXIT, format: EXIT\n and then remove the client from the list
	else if(strcmp(reply, EXIT_MSG) == 0){
        // Log and remove client
        fprintf(logger, "%s: EXIT\n", ONLINE_NAMES[*cur_client]);
		if(*cur_client == *online_clients-1){
			// If the client is the last one, just reduce the number of online_clients by one
			// and set the current client to 0
			*online_clients -= 1;
			*cur_client = 0;
			return;
		}
		// Shift every username left and reduce number of online_clients by one
		for(int i = *cur_client; i < *online_clients; i++){
			ONLINE_NAMES[i] = ONLINE_NAMES[i+1];
			ONLINE_SOCKETS[i] = ONLINE_SOCKETS[i+1];
		}
		*online_clients -= 1;
		return;
	}

	// Case for MESG, format: MESG:msg and to be printed as "src:msg"
	else if(strncmp(reply, MESG_MSG, strlen(MESG_MSG)) == 0){
		// extract message using two strtoks, no need to remove \n at the end because it needs to be printed
		char* msg = strtok(reply, ":");
		msg = strtok(NULL, ":");
		printf("%s:%s", ONLINE_NAMES[*cur_client], msg);
		fflush(stdout);
        // Also log the message printed
        fprintf(logger, "%s: %s", ONLINE_NAMES[*cur_client], msg);
		return;
	}

	// Case for LIST, format: LIST\n and to be sent as an enumeration of active clients followed by passive clients
	else if(strcmp(reply, LIST_MSG) == 0){
        // Log to file
        fprintf(logger, "%s: LIST\n", ONLINE_NAMES[*cur_client]);
		// Prepare a list of connected clients delimited by ':'
		char msg[1024];
		memset(msg, 0, 1024);
		for(int i = 0; i < *online_clients; i++){
			strcat(msg, ONLINE_NAMES[i]);
			strcat(msg, ":");
		}
        for(int i = 0; i < passive_clients - 1; i++){
            strcat(msg, PASSIVE_NAMES[i]);
            strcat(msg, ":");
        }
		strcat(msg, PASSIVE_NAMES[passive_clients-1]);
		strcat(msg, "\n");
        // Also log the message sent
        fprintf(logger, "%s", msg);
		// Send message to requesting client
		if(send(ONLINE_SOCKETS[*cur_client], msg, 1024, 0) == -1){ 
			close(ONLINE_SOCKETS[*cur_client]);
			exit(1);
		}
		return;
	}

    // Case for MSGC, format: MSGC:dest|msg and to be sent as MSGC:src|msg|idx
    else if(strncmp(reply, MSGC_MSG, strlen(MSGC_MSG)) == 0){
        // remove \n at the end
        if(reply[strlen(reply) - 1] == '\n'){
            reply[strlen(reply) - 1] = '\0';
        }
        char *msgc = strtok(reply, ":");
        char *rest = strtok(NULL, ":");
        if(strchr(rest, '|') == NULL){
            printf("INVALID MSGC\n");
            fflush(stdout);
            return;
        }
        char *dest = strtok(rest, "|");
        char *msg = strtok(NULL, "|");
        int found = 0;
        for(int i = 0; i < *online_clients; i++){
            if(strcmp(dest, ONLINE_NAMES[i]) == 0){
                found = 1;
                char msgc_reply[1024];
                memset(msgc_reply, 0, 1024);
                char hex_num[3];
                if(GLOBAL_IDX < 0xFE){
                    sprintf(&hex_num[0], "%02X", GLOBAL_IDX++);
                }
                else{
                    GLOBAL_IDX = 1;
                    sprintf(&hex_num[0], "%02X", GLOBAL_IDX++);
                }
                sprintf(msgc_reply, "%s%s|%s|%s\n", MSGC_MSG, ONLINE_NAMES[*cur_client], msg, hex_num); // Note that MSGC already has a colon
                if(send(ONLINE_SOCKETS[i], msgc_reply, 1024, 0) == -1){
                    close(ONLINE_SOCKETS[i]);
                    exit(1);
                }
                break;
            }
        }
        if(found == 0){
            // Search in passive clients
            for(int i = 0; i < passive_clients; i++){
                if(strcmp(dest, PASSIVE_NAMES[i]) == 0){
                    found = 1;
                    char msgc_reply[1024];
                    memset(msgc_reply, 0, 1024);
                    char hex_num[3];
                    if(GLOBAL_IDX < 0xFE){
                        sprintf(hex_num, "%02X", GLOBAL_IDX++);
                    }
                    else{
                        GLOBAL_IDX = 1;
                        sprintf(hex_num, "%02X", GLOBAL_IDX++);
                    }
                    sprintf(msgc_reply, "%s%s|%s|%s\n", MSGC_MSG, ONLINE_NAMES[*cur_client], msg, hex_num);
                    if(send(PASSIVE_SOCKETS[i], msgc_reply, 1024, 0) == -1){
                        close(PASSIVE_SOCKETS[i]);
                        exit(1);
                    }
                    break;
                }
            }
        }
        if(found == 0){
            // Log to file
            fprintf(logger, "INVALID RECIPIENT: %s\n", dest);
        }
        else{
            char hex_num[3];
            GLOBAL_IDX--;
            if(GLOBAL_IDX < 0xFE){
                sprintf(hex_num, "%02X", GLOBAL_IDX++);
            }
            else{
                GLOBAL_IDX = 1;
                sprintf(hex_num, "%02X", GLOBAL_IDX++);
            }
            // Log the message sent
            fprintf(logger, "%s->%s:%s:%s\n", ONLINE_NAMES[*cur_client], dest, hex_num, msg);
        }
        return;
    }

	// Case for INVALID CMD, format: anything else and to be printed as "INVALID CMD"
	else{
		// Invalid command, just print that and continue
		printf("INVALID CMD\n");
		fflush(stdout);
        // Log the invalid command
        fprintf(logger, "%s: INVALID CMD\n", ONLINE_NAMES[*cur_client]);
		return;
	}
}

// create a socket for clients
int create_socket(char* addr, int port, int NUM_TOTAL_CLIENTS) {

	// Create a TCP socket
	int server_sockfd;
	if((server_sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1){
		exit(1);
	}

	// Extra Code for faster restarts
	int yes = 1;
	if(setsockopt(server_sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1){
        close(server_sockfd);
        exit(1);
    }

	// Configure server
	struct sockaddr_in server_addrinfo;
	server_addrinfo.sin_family = AF_INET;
    server_addrinfo.sin_port = htons(port);
    if (inet_pton(AF_INET, addr, &server_addrinfo.sin_addr) <= 0) {
        close(server_sockfd);
        exit(1);
    }

	// Bind
	if(bind(server_sockfd, (struct sockaddr*) &server_addrinfo, sizeof(server_addrinfo)) == -1){
        close(server_sockfd);
        exit(1);
    }

	// Listen to max three
	if(listen(server_sockfd, NUM_TOTAL_CLIENTS) == -1){
		close(server_sockfd);
        exit(1);
	}

	return server_sockfd;
}

// Accept incoming connections
int client_connect(int socket_id, int client_num, char **CLIENT_NAMES, char **CLIENT_MODES) {
	struct sockaddr_in client_addrinfo;
	socklen_t sin_size = sizeof(client_addrinfo);

	// Accept
	int cur_sockfd;
	cur_sockfd = accept(socket_id, (struct sockaddr*) &client_addrinfo, &sin_size);
    if(cur_sockfd == -1){
        close(socket_id);
        exit(1);
    }

	// Send welcome message, i.e., "NAME\n"
	char msg[1024];
	memset(msg, 0, 1024);
	strcat(msg, NAME_MSG);
	if(send(cur_sockfd, msg, 1024, 0) == -1){
		close(socket_id);
		exit(1);
	}
	// Receive client name
	char* name = malloc(1024);
	memset(name, 0, 1024);
	if(recv(cur_sockfd, name, 1024, 0) == -1){
		close(socket_id);
		exit(1);
	}
	if(name[strlen(name) - 1] == '\n'){
		name[strlen(name) - 1] = '\0';
	}
	CLIENT_NAMES[client_num] = name;

    // Send the "MODE\n" message
    memset(msg, 0, 1024);
    strcat(msg, MODE_MSG);
    if(send(cur_sockfd, msg, 1024, 0) == -1){
        close(socket_id);
        exit(1);
    }
    // Receive client mode
    char* mode = malloc(1024);
    memset(mode, 0, 1024);
    if(recv(cur_sockfd, mode, 1024, 0) == -1){
        close(socket_id);
        exit(1);
    }
    if(mode[strlen(mode) - 1] == '\n'){
        mode[strlen(mode) - 1] = '\0';
    }
    CLIENT_MODES[client_num] = mode;

	// Maybe add a check with inet_ntop?
	return cur_sockfd;
}

int main(int argc, char *argv[]){
    if (argc != 4){
		printf("Use 3 cli arguments\n");
		return -1;
	}
	
	// extract the address and port from the command line arguments and open log.txt
	char addr[INET6_ADDRSTRLEN];
	int port, NUM_TOTAL_CLIENTS;
	strcpy(addr, argv[1]);
	port = atoi(argv[2]);
    NUM_TOTAL_CLIENTS = atoi(argv[3]);
	int socket_id = create_socket(addr, port, NUM_TOTAL_CLIENTS);
    logger = fopen("./log.txt", "w");

    // Initialize the global arrays
    int CLIENT_SOCKETS[NUM_TOTAL_CLIENTS];
    char *CLIENT_NAMES[NUM_TOTAL_CLIENTS];
    char *CLIENT_MODES[NUM_TOTAL_CLIENTS];
    int ONLINE_CLIENTS = 0;
	for(int i = 0; i < NUM_TOTAL_CLIENTS; i++){
		CLIENT_SOCKETS[i] = client_connect(socket_id, i, CLIENT_NAMES, CLIENT_MODES);
        if(strcmp(CLIENT_MODES[i], "active") == 0){
            ONLINE_CLIENTS++;
        }
        fprintf(logger, "CLIENT: %s; MODE: %s\n", CLIENT_NAMES[i], CLIENT_MODES[i]);
	}
    int PASSIVE_CLIENTS = NUM_TOTAL_CLIENTS - ONLINE_CLIENTS;
    int ONLINE_SOCKETS[ONLINE_CLIENTS];
    int PASSIVE_SOCKETS[PASSIVE_CLIENTS];
    char *ONLINE_NAMES[ONLINE_CLIENTS];
    char *PASSIVE_NAMES[PASSIVE_CLIENTS];
    for(int i = 0, j = 0, k = 0; i < NUM_TOTAL_CLIENTS; i++){ // DID NOT KNOW THIS! WOW!
        if(strcmp(CLIENT_MODES[i], "active") == 0){
            ONLINE_SOCKETS[j] = CLIENT_SOCKETS[i];
            ONLINE_NAMES[j] = CLIENT_NAMES[i];
            j++;
        }
        else{
            PASSIVE_SOCKETS[k] = CLIENT_SOCKETS[i];
            PASSIVE_NAMES[k] = CLIENT_NAMES[i];
            k++;
        }
    }

	int cur_client = 0;
	while(ONLINE_CLIENTS > 0){
		
		// POLL\n to cur_client
		char msg[1024];
		memset(msg, 0, 1024);
		strcat(msg, POLL_MeSG);
		if(send(ONLINE_SOCKETS[cur_client], msg, 1024, 0) == -1){
			close(ONLINE_SOCKETS[cur_client]);
			exit(1);
		}

		// Receive command from client
		char reply[1024];
		memset(reply, 0, 1024);
		if(recv(ONLINE_SOCKETS[cur_client], reply, 1024, 0) == -1){
			close(ONLINE_SOCKETS[cur_client]);
			exit(1);
		}

		// Process reply
		processReply(reply, &cur_client, &ONLINE_CLIENTS, PASSIVE_CLIENTS, ONLINE_SOCKETS, ONLINE_NAMES, PASSIVE_SOCKETS, PASSIVE_NAMES);

		// Move on to next client
		if((strcmp(reply, EXIT_MSG) != 0) && ONLINE_CLIENTS > 0){
			// No need to move to next client if the current client has exited
			// The case of last client is handled in processReply
			cur_client = (cur_client + 1) % ONLINE_CLIENTS;
		}
	}
    // Close passive sockets once all active clients have exited
    for(int i = 0; i < PASSIVE_CLIENTS; i++){
        close(PASSIVE_SOCKETS[i]);
    }
    printf("SERVER TERMINATED: EXITING......\n");
	fflush(stdout);
    fprintf(logger, "SERVER TERMINATED: EXITING......\n");
    fclose(logger);
    close(socket_id);
    return 0;    
}

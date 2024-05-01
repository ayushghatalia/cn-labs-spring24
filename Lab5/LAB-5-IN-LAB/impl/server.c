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
#include <semaphore.h>

char *NAME_MSG = "NAME\n", *EXIT_MSG = "EXIT\n", *GRPS_MSG = "GRPS:";
char *BCST_MSG = "BCST:", *LIST_MSG = "LIST\n", *MCST_MSG = "MCST:";
char *MSGC_MSG = "MSGC:", *HIST_MSG = "HIST\n", *CAST_MSG = "CAST:";

FILE *logger;
sem_t global_var_sem;

char* password;
char CLIENT_MODES[1024];
char *CLIENT_NAMES[1024];
int CLIENT_SOCKETS[1024];
pthread_t CLIENT_THREADS[1024];
int CLIENT_NUM = 0;

typedef struct{
    char *group_name;
    char *modes;
    char **names;
    int *sockets;
    pthread_t *threads;
    int num;
} Group;

Group GROUPS[1024];
int GROUP_NUM = 0;

// Process the reply from the client
int processReply(char *msg, int *cur_socket){
    
    // Find index of the current client
    int cur_client_num = 0;
    // sem_wait(&global_var_sem);
    for(int i = 0; i < CLIENT_NUM; i++){
        if(CLIENT_SOCKETS[i] == *cur_socket){
            cur_client_num = i;
            break;
        }
    }
    int *ret_status = (int *)malloc(sizeof(int));

    // Log  message to file
    fprintf(logger, "%s-%s", CLIENT_NAMES[cur_client_num], msg);
    fflush(logger);

	// Case for EXIT, format: EXIT\n and then remove the client from the global list and any groups
	if(strcmp(msg, EXIT_MSG) == 0){

        // Check if the client is a prt of any groups and remove them from the groups
        if(GROUP_NUM > 0){

            // Iterate through all the groups
            for(int i = 0; i < GROUP_NUM; i++){

                // Iterate through all the members of the group
                for(int j = 0; j < GROUPS[i].num; j++){

                    // If the client is found, remove it from the group and break the loop because the client can only be once in one group
                    if(*cur_socket == GROUPS[i].sockets[j]){
                        for(int k = j; k < GROUPS[i].num; k++){
                            GROUPS[i].modes[k] = GROUPS[i].modes[k+1];
                            GROUPS[i].names[k] = GROUPS[i].names[k+1];
                            GROUPS[i].sockets[k] = GROUPS[i].sockets[k+1];
                            GROUPS[i].threads[k] = GROUPS[i].threads[k+1];
                        }
                        GROUPS[i].num--;
                        break;
                    }
                }

                // Check if the group is empty and remove it
                if(GROUPS[i].num == 0){
                    for(int j = i; j < GROUP_NUM; j++){
                        GROUPS[j] = GROUPS[j+1];
                    }
                    GROUP_NUM--;
                }
            }
        }

        // close the socket
        close(*cur_socket);

		if(cur_client_num == CLIENT_NUM-1){
			// If the client is the last one, just reduce the number of online_clients by one
			// and set the current client to 0
			CLIENT_NUM -= 1;
			return 2; // 2 for EXIT
		}
		// Shift every username left and reduce number of online_clients by one
		for(int i = cur_client_num; i < CLIENT_NUM; i++){
            CLIENT_MODES[i] = CLIENT_MODES[i+1];
			CLIENT_NAMES[i] = CLIENT_NAMES[i+1];
			CLIENT_SOCKETS[i] = CLIENT_SOCKETS[i+1];
            CLIENT_THREADS[i] = CLIENT_THREADS[i+1];
		}
		CLIENT_NUM -= 1;

        // Prepare a list of connected clients delimited by ':'
		char msg[1024];
		memset(msg, 0, 1024);
        strcat(msg, "LIST-");
		for(int i = 0; i < CLIENT_NUM - 1; i++){
			strcat(msg, CLIENT_NAMES[i]);
            if(CLIENT_MODES[i] == 1){
                strcat(msg, "|r");
            }
            else{
                strcat(msg, "|n");
            }
			strcat(msg, ":");
		}
		strcat(msg, CLIENT_NAMES[CLIENT_NUM-1]);
        if(CLIENT_MODES[CLIENT_NUM-1] == 1){
            strcat(msg, "|r");
        }
        else{
            strcat(msg, "|n");
        }
		strcat(msg, "\n"); // Can log output on next line
		// Send message to all clients
        for(int i = 0; i < CLIENT_NUM; i++){
            if(send(CLIENT_SOCKETS[i], msg, 1024, 0) == -1){ 
                ;
            }
        }
		return 2;
	}

	// Case for LIST, format: LIST\n and to be sent as client1:client2:...:clientn\n
	else if(strcmp(msg, LIST_MSG) == 0){

		// Prepare a list of connected clients delimited by ':'
		char msg[1024];
		memset(msg, 0, 1024);
        strcat(msg,"LIST-");
		for(int i = 0; i < CLIENT_NUM - 1; i++){
			strcat(msg, CLIENT_NAMES[i]);
            strcat(msg,"|");
            msg[strlen(msg)] = CLIENT_MODES[i];
			strcat(msg, ":");
		}
		strcat(msg, CLIENT_NAMES[CLIENT_NUM-1]);
        strcat(msg,"|");
        msg[strlen(msg)] = CLIENT_MODES[CLIENT_NUM-1];
		strcat(msg, "\n");
         // Can log output on next line
		// Send message to requesting client
		if(send(*cur_socket, msg, 1024, 0) == -1){ 
			close(*cur_socket);
			*ret_status = 1;
            pthread_exit(ret_status);(1);
		}

		return 0;
	}

    // Case for MSGC, format: MSGC:dest:msg and to be sent as src:msg
    else if(strncmp(msg, MSGC_MSG, strlen(MSGC_MSG)) == 0){
        // remove \n at the end
        if(msg[strlen(msg) - 1] == '\n'){
            msg[strlen(msg) - 1] = '\0';
        }
        char *msgc = strtok(msg, ":");
        char *dest = strtok(NULL, ":");
        char *msg = strtok(NULL, ":");
        int found = 0;
        for(int i = 0; i < CLIENT_NUM; i++){
            if(strcmp(dest, CLIENT_NAMES[i]) == 0){
                found = 1;
                char msgc_reply[1024];
                memset(msgc_reply, 0, 1024);
                sprintf(msgc_reply, "%s:%s\n", CLIENT_NAMES[cur_client_num], msg);
                if(send(CLIENT_SOCKETS[i], msgc_reply, 1024, 0) == -1){
                    close(CLIENT_SOCKETS[i]);
                    *ret_status = 1;
                    pthread_exit(ret_status);
                }
                break;
            }
        }
        if(found == 0){
            char msgc_reply[1024];
            memset(msgc_reply, 0, 1024);
            strcat(msgc_reply, "USER NOT FOUND\n");
            if(send(*cur_socket, msgc_reply, 1024, 0) == -1){
                close(*cur_socket);
                *ret_status = 1;
                pthread_exit(ret_status);
            }
        }
        
        return 0;
    }

    // Case for GRPS, format: GRPS:u1,u2,...,un:<groupname>\n, creates a group with the given name
    else if(strncmp(msg, GRPS_MSG, strlen(GRPS_MSG)) == 0){
        // remove \n at the end
        if(msg[strlen(msg) - 1] == '\n'){
            msg[strlen(msg) - 1] = '\0';
        }
        char *tok = strtok(msg, ":");
        char *users = strtok(NULL, ":");
        char *group = strtok(NULL, ":");

        // Create a new group
        Group *new_group = (Group *)malloc(sizeof(Group));
        // Allocate memory to all the names, sockets and threads
        new_group->modes = (char *)malloc(1024 * sizeof(char));
        new_group->names = (char **)malloc(1024 * sizeof(char *));
        new_group->sockets = (int *)malloc(1024 * sizeof(int));
        new_group->threads = (pthread_t *)malloc(1024 * sizeof(pthread_t));
        new_group->group_name = (char *)malloc(1024 * sizeof(char));
        memset(new_group->group_name, 0, 1024);
        strcat(new_group->group_name, group);
        new_group->num = 0;

        // Iterate through the users and add them to the group
        char *user = strtok(users, ",");
        int valid = 1;

        while(user != NULL){
            int found = 0;
            new_group->names[new_group->num] = user;

            // Find the user in the list of clients
            for(int i = 0; i < CLIENT_NUM; i++){
                if(strcmp(user, CLIENT_NAMES[i]) == 0){
                    found = 1;
                    new_group->modes[new_group->num] = CLIENT_MODES[i];
                    new_group->sockets[new_group->num] = CLIENT_SOCKETS[i];
                    new_group->threads[new_group->num] = CLIENT_THREADS[i];
                    break;
                }
            }

            // If the user is not found, send an error message
            if(found == 0){
                char grps_reply[1024];
                memset(grps_reply, 0, 1024);
                strcat(grps_reply, "INVALID USERS LIST\n");
                if(send(*cur_socket, grps_reply, 1024, 0) == -1){
                    close(*cur_socket);
                    *ret_status = 1;
                    pthread_exit(ret_status);
                }
                valid = 0;
                break;
            }
            new_group->num++;
            user = strtok(NULL, ","); // Get the next user
        }

        // If the group is valid, add it to the list of groups
        if(valid == 1){
            int exists = 0;
            int idx = -1;
            // Check if the same groupname exists
            if(GROUP_NUM > 0){
                // Iterate through all the groups
                for(int i = 0; i < GROUP_NUM; i++){
                    if(strcmp(GROUPS[i].group_name, group) == 0){
                        exists = 1;
                        idx = i;
                    }
                }
            }

            // When group exists, override with this one
            if(exists == 1){
                GROUPS[idx] = *new_group;    
            }
            else{
                GROUPS[GROUP_NUM] = *new_group;
                GROUP_NUM++;
            }
            char grps_reply[1024];
            memset(grps_reply, 0, 1024);
            sprintf(grps_reply, "GROUP %s CREATED\n", group);
            if(send(*cur_socket, grps_reply, 1024, 0) == -1){
                close(*cur_socket);
                *ret_status = 1;
                pthread_exit(ret_status);
            }
        }

        return 0;
    }

    // Case for BCST, format: BCST:msg\n, sends a message (src:msg) to all clients except the current
    else if(strncmp(msg, BCST_MSG, strlen(BCST_MSG)) == 0){
        // remove \n at the end
        if(msg[strlen(msg) - 1] == '\n'){
            msg[strlen(msg) - 1] = '\0';
        }
        char *bcst = strtok(msg, ":");
        char *mesg = strtok(NULL, ":");
        char msg_to_send[1024];
        memset(msg_to_send, 0, 1024); // IMPORTANT: Reset the message to all 0s
        sprintf(msg_to_send, "%s:%s\n", CLIENT_NAMES[cur_client_num], mesg);

        // Send the message to all clients except the current
        for(int i = 0; i < CLIENT_NUM; i++){
            if(i != cur_client_num){
                if(send(CLIENT_SOCKETS[i], msg_to_send, 1024, 0) == -1){
                    close(CLIENT_SOCKETS[i]);
                    *ret_status = 1;
                    pthread_exit(ret_status);
                }
            }
        }

        return 0;
    }
    else if(strncmp(msg, CAST_MSG, strlen(CAST_MSG)) == 0){
        // remove \n at the end
        if(msg[strlen(msg) - 1] == '\n'){
            msg[strlen(msg) - 1] = '\0';
        }
        char *cast = strtok(msg, ":");
        char *mesg = strtok(NULL, ":");
        char msg_to_send[1024];
        memset(msg_to_send, 0, 1024); // IMPORTANT: Reset the message to all 0s
        sprintf(msg_to_send, "%s:%s\n", CLIENT_NAMES[cur_client_num], mesg);
        char mode = CLIENT_MODES[cur_client_num];
        // Send the message to all clients except the current
        for(int i = 0; i < CLIENT_NUM; i++){
            if(i != cur_client_num){
                if(CLIENT_MODES[i] == mode) {
                    if(send(CLIENT_SOCKETS[i], msg_to_send, 1024, 0) == -1){
                        close(CLIENT_SOCKETS[i]);
                        *ret_status = 1;
                        pthread_exit(ret_status);
                    }
                }
            }
        }

        return 0;
    }

    // Case for MCST, format: MCST:groupname:msg\n, sends a message (src:msg) to all clients in the group
    else if(strncmp(msg, MCST_MSG, strlen(MCST_MSG)) == 0){
        // remove \n at the end
        if(msg[strlen(msg) - 1] == '\n'){
            msg[strlen(msg) - 1] = '\0';
        }
        char *mcst = strtok(msg, ":");
        char *group = strtok(NULL, ":");
        char *mesg = strtok(NULL, ":");
        int found = 0;
        for(int i = 0; i < GROUP_NUM; i++){
            if(strcmp(group, GROUPS[i].group_name) == 0){
                found = 1;
                char msg_to_send[1024];
                memset(msg_to_send, 0, 1024); // IMPORTANT: Reset the message to all 0s
                sprintf(msg_to_send, "%s:%s\n", CLIENT_NAMES[cur_client_num], mesg);
                for(int j = 0; j < GROUPS[i].num; j++){
                    if(send(GROUPS[i].sockets[j], msg_to_send, 1024, 0) == -1){
                        close(GROUPS[i].sockets[j]);
                        *ret_status = 1;
                        pthread_exit(ret_status);
                    }
                }
                break;
            }
        }
        if(found == 0){
            char msgc_reply[1024];
            memset(msgc_reply, 0, 1024);
            sprintf(msgc_reply, "GROUP %s NOT FOUND\n", group);
            if(send(*cur_socket, msgc_reply, 1024, 0) == -1){
                close(*cur_socket);
                *ret_status = 1;
                pthread_exit(ret_status);
            }
        }
        
        return 0;
    }

    // Case for HIST\n, format: HIST\n, sends the history of the chat
    else if(strcmp(msg, HIST_MSG) == 0){

        // Reset the reader
        fclose(logger);
        logger = fopen("./log.txt", "r");
        fseek(logger, 0, SEEK_SET);

        // Read the log file and add to a string
        char msg_to_send[1024];
        char line[1024];
        memset(line, 0, 1024);
        while(fgets(line, 1024, logger) != NULL){
            strcat(msg_to_send, line);
            memset(line, 0, 1024);
        }
        if(send(*cur_socket, msg_to_send, 1024, 0) == -1){
            close(*cur_socket);
            *ret_status = 1;
            pthread_exit(ret_status);
        }

        // Reset the reader to initial state
        fclose(logger);
        logger = fopen("./log.txt", "a");
        
        return 0;
    }

	// Case for INVALID CMD, format: anything else and to be printed as "INVALID CMD"
	else{
		// Invalid command, just print that and continue
		printf("INVALID CMD\n");
		fflush(stdout);
		return 0;
	}

}

// Listens to one client in a loop
void* listenToClient(void* args){

    int client = *((int *)args);
    // Search for client and assign it the pthread_self()
    for(int i = 0; i < CLIENT_NUM; i++){
        if(CLIENT_SOCKETS[i] == client){
            CLIENT_THREADS[i] = pthread_self();
            break;
        }
    }

    int *ret_status = (int *)malloc(sizeof(int));

	while(1){

		// Receive command from client
        int recv_count;
		char msg[1024];
		memset(msg, 0, 1024);
		if((recv_count = recv(client, msg, 1024, 0)) == -1){
			close(client);
            *ret_status = 1;
			pthread_exit(ret_status);
		}

        // If client has closed the connection, then exit
        if(recv_count == 0){
            close(client);
            exit(0);
        }

		// Process reply
        sem_wait(&global_var_sem);
		processReply(msg, &client); // passing the socket id of the client
        sem_post(&global_var_sem);
	}
}

// create a socket for clients
int create_socket(char* addr, int port) {

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

	// Listen to max possible SOMAXCONN = 4096
	if(listen(server_sockfd, SOMAXCONN) == -1){
		close(server_sockfd);
        exit(1);
	}

	return server_sockfd;
}

// Accept incoming connections
void* client_connect(void* args) {

    int socket_id = *((int *)args);
    int *ret_status = (int *)malloc(sizeof(int));
	
    while(1){
        struct sockaddr_in client_addrinfo;
        socklen_t sin_size = sizeof(client_addrinfo);

        // Accept
        int cur_sockfd;
        cur_sockfd = accept(socket_id, (struct sockaddr*) &client_addrinfo, &sin_size); // accept is a blocking call!
        if(cur_sockfd == -1){
            close(socket_id);
            *ret_status = 1;
            pthread_exit(ret_status);
        }

        // Receive client name, 1st message from client
        char* name = (char *)malloc(sizeof(char) * 1024);
        memset(name, 0, 1024);
        if(recv(cur_sockfd, name, 1024, 0) == -1){
            close(cur_sockfd);
            *ret_status = 1;
            pthread_exit(ret_status);
        }
        if(name[strlen(name) - 1] == '\n'){
            name[strlen(name) - 1] = '\0';
        }
        char* name1;
        char* type;
        char type1;
        name1 = strtok(name,"|");
        type = strtok(NULL,"|");
        type1 = type[0];
        if(strncmp(type,"r",1) == 0) {
            char* pw = (char *)malloc(sizeof(char) * 1024);
            memset(pw, 0, 1024);
            if(recv(cur_sockfd, pw, 1024, 0) == -1){
                close(cur_sockfd);
                *ret_status = 1;
                pthread_exit(ret_status);
            }
            // strcat(pw,"\n");
            if(strcmp(pw,password) == 0) {
                char reply[1024];
                memset(reply,0,1024);
                strcat(reply,"PASSWORD ACCEPTED\n");
                if(send(cur_sockfd, reply, 1024, 0) == -1){
                    close(cur_sockfd);
                    *ret_status = 1;
                    pthread_exit(ret_status);
                }
                // get the lock for global variables
                sem_wait(&global_var_sem);
                // set all the global variables
                CLIENT_MODES[CLIENT_NUM] = type1;
                CLIENT_NAMES[CLIENT_NUM] = name1;
                CLIENT_SOCKETS[CLIENT_NUM] = cur_sockfd;
                CLIENT_NUM++;
                // release the lock for global variables
                sem_post(&global_var_sem);

                // Create a new thread for this client
                pthread_t new_client;
                pthread_attr_t new_client_attr;
                pthread_attr_init(&new_client_attr);
                int *client_arg = malloc(sizeof(*client_arg));
                *client_arg = cur_sockfd;
                pthread_create(&new_client, &new_client_attr, listenToClient, client_arg);
            }
            else {
                char reply[1024];
                memset(reply,0,1024);
                strcat(reply,"PASSWORD REJECTED\n");
                if(send(cur_sockfd, reply, 1024, 0) == -1){
                    close(cur_sockfd);
                    *ret_status = 1;
                    pthread_exit(ret_status);
                }
                close(cur_sockfd);
            }
        }
        else {
            // get the lock for global variables
            sem_wait(&global_var_sem);
            // set all the global variables
            CLIENT_MODES[CLIENT_NUM] = type1;
            CLIENT_NAMES[CLIENT_NUM] = name1;
            CLIENT_SOCKETS[CLIENT_NUM] = cur_sockfd;
            CLIENT_NUM++;
            // release the lock for global variables
            sem_post(&global_var_sem);

            // Create a new thread for this client
            pthread_t new_client;
            pthread_attr_t new_client_attr;
            pthread_attr_init(&new_client_attr);
            int *client_arg = malloc(sizeof(*client_arg));
            *client_arg = cur_sockfd;
            pthread_create(&new_client, &new_client_attr, listenToClient, client_arg);
        }
    }

    // Join client threads that have exited
    // No need because on recv_count == 0, direct exit() is called
    // instead of pthread_exit()
}

int main(int argc, char *argv[]){
    if (argc != 4){
		printf("Use 3 cli arguments\n");
		return -1;
	}
	
	// extract the address and port from the command line arguments and open log.txt
	char addr[INET6_ADDRSTRLEN];
	int port;
    password = argv[3];
	strcpy(addr, argv[1]);
	port = atoi(argv[2]);
	int socket_id = create_socket(addr, port);

    // Empty the file
    logger = fopen("./log.txt", "w");

    // Initialize the semaphore
    sem_init(&global_var_sem, 0, 1);

    // Empty global variables using memset
    memset(CLIENT_NAMES, 0, 1024);
    memset(CLIENT_SOCKETS, 0, 1024);
    memset(CLIENT_THREADS, 0, 1024);
    memset(GROUPS, 0, 1024);

    // Create the thread that accepts clients in an infinite loop
    pthread_t client_acceptor;
    pthread_attr_t client_acceptor_attr;
    pthread_attr_init(&client_acceptor_attr);
    int *arg = malloc(sizeof(*arg));
    *arg = socket_id;
    pthread_create(&client_acceptor, &client_acceptor_attr, client_connect, arg);

    // This thread will never join? 
    void *_ret_status;
    pthread_join(client_acceptor, &_ret_status); // Blocked here
    int ret_status = *((int*) _ret_status);
    if(ret_status == 1){
        printf("Error has occured\n");
        fflush(stdout);
    }
    close(socket_id);
    fprintf(logger, "SERVER TERMINATED: EXITING......\n");
    fclose(logger);
    return 0;
}

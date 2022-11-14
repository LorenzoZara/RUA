#include "../inc/libraries.h"
#include"../inc/config.h"
#include "../inc/func.h"
#include "../inc/interrupt.h"
#include "../inc/cmd.h"
#include "../inc/types.h"



int socket_fd;
int num_client = 0;
int *free_port;  //ports bitmap

int main(int argc, char **argv){

    /* MAIN PROCESS SERVER STACK */

        pid_t child_pids[MAX_CLIENTS];
        pid_t parent_pid = getpid();

        /* SHARING MEMORY */

            // creating bitmap for socket ports
            free_port = (int *) mmap(NULL, MAX_CLIENTS, PROT_READ|PROT_WRITE, MAP_ANONYMOUS | MAP_SHARED, 0, 0);
            if (free_port == NULL){
                printf("mmap error while sharing memory for ports bitmap.\n");
                printf("Server shutdown.\n");
                exit(-1);
            }

            // initializzating bitmap for socket ports
            for (int i = 0; i < MAX_CLIENTS; i++){
                free_port[i] = 0;
            }

    /*  SIGNAL MANAGEMENT */

        struct sigaction act_int;
        struct sigaction act_chld;
        sigset_t set_int;
        sigset_t set_chld;

        sigfillset(&set_int);
        act_int.sa_sigaction = interrupt_handler;
        act_int.sa_mask = set_int;
        act_int.sa_flags = 0;
        sigaction(SIGINT, &act_int, NULL);

        sigfillset(&set_chld);
        act_chld.sa_sigaction = child_death_handler;
        act_chld.sa_mask = set_chld;
        act_chld.sa_flags = 0;
        sigaction(SIGCHLD, &act_chld, NULL);

    /* WELCOMING SOCKET CREATION */

        char buffer[BUFFER_SIZE];
        int len = sizeof(client_addr);

        printf("\nStarting server...\n");

        socket_fd = create_socket(PORT);

    /* MAIN PROCESS SERVER ROUTINE */

        while (1) {     rec:

            
            // buffer flushing
            bzero(buffer, BUFFER_SIZE);
            // struct client_addr flushing     
            bzero(&client_addr, sizeof(client_addr));

            // buffer receiving
            if (recvfrom(socket_fd, (char *) buffer, sizeof(buffer), 0, (SA *) &client_addr, &len) == -1){
                if (errno == EINTR) goto rec;
                printf("rcvfrom() error, error: %s .\n", strerror(errno));
                bzero(buffer, BUFFER_SIZE);
            }

            int client_port = cmd_send_port(socket_fd, client_addr, free_port);

            if(client_port == -1){
                goto rec;
            }
            num_client ++; printf("Number of clients: %d\n\n",num_client);

            pid_t pid = fork();
        

            if (pid == -1){
                printf("Fork error while creating process for new client");
            }

            if( pid == 0 ){

                int ret; 

                signal(SIGCHLD,SIG_IGN);
                socket_fd_child = create_socket(client_port);

                /* CHILD PROCESS ROUTINE */
            
                    while (1){
                        
                        // buffer flusing
                        bzero(buffer,BUFFER_SIZE);
                        // buffer receiving
                        recvfrom(socket_fd_child, buffer, sizeof(buffer), 0, (SA *) &client_addr, &len);
                        // check
                            if (ret == -1){
                                printf("recvfrom error while receiving request from client connected on port %d.\n", client_port);
                                printf("Disconnecting client...\n");
                                close(socket_fd_child);
                                exit(-1);
                            }

                        char token[50];
                        char size[50];                      
                        strcpy(token,strtok(buffer," "));

                      /* LIST REQUEST */

                        if(strcmp("list", token) == 0){
                            printf("Processing request from client connected on port %d.\n", client_port);
                            cmd_list(socket_fd_child,client_addr);
                        }
else 
                      /* GET REQUEST */

                        if(strcmp("get", token) == 0){

        					printf("Processing request from client connected on port %d.\n", client_port);
                            strcpy(token,strtok(NULL," "));
                            printf("GET request for %s\n", token);
                            cmd_send_packets(socket_fd_child, token, client_addr, len);

                        }
else 
                      /* PUT REQUEST */

                        if(strcmp("put", token) == 0){

        					printf("Processing request from client connected on port %d.\n", client_port);
                            strcpy(token,strtok(NULL," "));
                            strcpy(size,strtok(NULL,""));
                            if (!cmd_corr_put(size,socket_fd_child,client_addr)){
                                continue;
                            }
                            else {
                                printf("PUT request. Receiving %s (%s bytes) from port %d\n", token, size, client_port);
                                cmd_recv_packets(token,size,socket_fd_child,client_addr);
                            }
				        }
else 
                      /* EXIT REQUEST */
                        
                        if(strcmp("exit", token) == 0){
                        
                            printf("Client exit from port %d.\n\n",client_port);
                            fflush(stdout);
                            close(socket_fd_child);
                            free_port[client_port - PORT] = 0;  //free bitmap slot 
                            exit(0);
                        
                        } /*else {
                            printf("Comando del cliente su porta: %d nullo\n\n",client_port);
                        }*/
                    }
            }
        }
        close(socket_fd);
        return 0;
}
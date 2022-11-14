/* INCLUDES */
    #include "../inc/libraries.h"
    #include"../inc/config.h"
    #include "../inc/func.h"
    #include "../inc/interrupt.h"
    #include "../inc/cmd.h"
    #include "../inc/types.h"
    #include "../inc/threads.h"

/* DEFINES */

    #define fflush(stdin) while ( getchar() != '\n') { }

/* CLIENT BSS AND DATA */

    // file descriptor for socket
    int socket_fd;
    // server address
    struct sockaddr_in server_addr;
    // server address length
    int len = sizeof(server_addr);



int main(int argc, char **argv){

    printf("Launching RUA client software...\n");
    printf("Initializating environment...\n");
    printf("Trying to connect to server...\n");
    

    /* CLIENT STACK */

        // checking values variable 
        int ret;

        // connection waiting variables
        pthread_t connection_checker_tid;
        int *connected = (int *) malloc(sizeof(int)); *connected = 0;

        // sending/receiving buffer
        char buffer[BUFFER_SIZE];

        // integer response from server variable
        int response; char str_response[4];

        // signal management variables
        struct sigaction SIGINT_act;
        sigset_t SIGINT_set;

    /* SIGNAL MANAGEMENT */

        sigfillset(&SIGINT_set);
        SIGINT_act.sa_sigaction = SIGINT_handler;
        SIGINT_act.sa_mask = SIGINT_set;
        SIGINT_act.sa_flags = 0;
        sigaction(SIGINT, &SIGINT_act, NULL);

    /* CONNECTION WAITING */

        printf("%d seconds left for connection...\n", TIMEOUT_CONNECTION);
        ret = pthread_create(&connection_checker_tid, NULL, connection_checker, (void *) connected);
        if (ret == -1){
            printf("pthread_create() error while launcing connection checker thread.\n");
            exit(-1);
        }

    /* WELCOMING SOCKET CREATION */

        // socket initializzation
        socket_fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
        if (socket_fd == -1){
            printf("socket() error while creating socket.\n"
                    "Error: %s\n", strerror(errno));
            exit(-1);
        }
	    // server_addr flushing
	    bzero(&server_addr, len);
        // server_addr setting
	    server_addr.sin_family = AF_INET; 
	    server_addr.sin_addr.s_addr = inet_addr(ADDRESS); 
	    server_addr.sin_port = htons(PORT);

    /* SENDING HELLO REQUEST */ retry_send_hello:

        // buffer flushing
        bzero(buffer, BUFFER_SIZE);
        // buffer setting
        strcpy(buffer, "hello");
        // buffer sending
        ret = sendto(socket_fd, buffer, BUFFER_SIZE, 0, (SA *) &server_addr, len);
        // check
        if (ret == -1){
            if (errno == EINTR) goto retry_send_hello;
            else {
                printf("sendto() error while sending 'hello' request.\n"
                        "Client closing...\n");
                close(socket_fd);
                exit(-1);
            }
        }


    /* GET PERMANENT CONNECTION */ retry_receive_port_number:

        // buffer flushing
        bzero(buffer, BUFFER_SIZE);
        // buffer receiving : receiving new port number
        ret = recvfrom(socket_fd, buffer, BUFFER_SIZE, 0, (SA *) &server_addr, &len);
        // check
        if (ret == -1){
            if (errno == EINTR) goto retry_receive_port_number;
            else {
                printf("recvfrom() error while receiving new port number.\n"
                        "Client closing...\n");
                close(socket_fd); 
                exit(-1);
            }
        }
        // response = new port number
        response = atoi(buffer);
        // check
        if (response == SERVICE_UNAVAILABLE){
            printf("Server is out of service...\n"
                    "Closing client...\n");
            close(socket_fd);
            exit(-1);
        }
        // close welcoming connection
        close(socket_fd);
    
    /* PERMANENT SOCKET CREATION */

        // socket initialization
        socket_fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
        // check
        if (socket_fd == -1){
            printf("Cannot create the socket.\n");
            exit(-1);
        }

        // server_addr setting
        server_addr.sin_port = htons(response);

        *connected = 1;

        printf("Client connected succesfully!\n");
        printf("\n*** WELCOME TO RUA ***\n");
        printf("With RUA you can download and upload files.\n\n");

        // display commands
        display();


    /* CLIENT ROUTINE */

        while (1) {

            char token[50];
            char size[50];

            // buffer flushing
            bzero(buffer, BUFFER_SIZE);

            // buffer setting from stdin
            scanf("%[^\n]", buffer); fflush(stdin);
            
            /* LIST REQUEST */
                
                // input check
                if ((strcmp(buffer, "list") == 0) || (strcmp(buffer, "list ") == 0)){

                    display_request_message("list", "\0", "\0");

                    // send request and print server available files
                    cmd_list(socket_fd, buffer, server_addr, len);

                    // display available commands
                    display();

                }
else
            /* GET REQUEST */

                // input check
                if (strncmp(buffer, "get", 3) == 0){

                    if (strcmp(buffer, "get") == 0 || strcmp(buffer, "get ") == 0){
                        printf("Wrong synthax: please enter a file name.\n");
                        display();
                        continue;
                    }

                    display_request_message("get", buffer + 4, "\0");

                    // buffer sending "get file_name"
                    ret = sendto(socket_fd, buffer, BUFFER_SIZE, 0, (SA *) &server_addr, len);

                    // check
                    if (ret == -1){
                    printf("sendto() error while sending put command.\n"
                            "Client closing...\n");
                    close(socket_fd);
                    exit(-1);
                    }

retryReceive:       
                    bzero(buffer, BUFFER_SIZE);
                    // server replies with 200 file_name file_size...
                    ret = recvfrom(socket_fd, buffer, BUFFER_SIZE, 0, (SA *) &server_addr, &len);
                        // check
                        if (ret == -1){
                            printf("recvfrom() error while sending put command.\n"
                                "Client closing...\n");
                            close(socket_fd);
                            exit(-1);
                        }


                    strncpy(str_response, buffer, 4);
                    response = atoi(str_response);

                    // ... handling cases in which it doesn't
                    if (response != OK){
                        if (response == NOT_FOUND){
                            printf("File indicated is not stored in server.\n");
                            display_response_message(str_response, "\0", "\0");
                            display(); continue;
                        }else if (response == BAD_REQUEST){
                            printf("Wrong synthax.\n");
                            display(); continue;
                        }else if (response == SERVICE_UNAVAILABLE){
                            printf("Server has shutdown.\nClient closing...\n");
                            close(socket_fd);
                            exit(-1);
                        } else goto retryReceive;
                    }

                    // getting downloading infos
                    char fileName[128];
                    char file_size_str[128];

                    strtok(buffer, " ");
                    strcpy(fileName, strtok(NULL, " "));
                    strcpy(file_size_str, strtok(NULL, "\0"));

                    
                    display_response_message(strtok(str_response, " "), fileName, file_size_str);

                    if (strcmp(fileName, "") == 0){
                        printf("Wrong synthax: please enter a file name.\n");
                        display();
                        continue;
                    }

                    // start pachet recieving
                    cmd_recv_packets(fileName, file_size_str, socket_fd, server_addr);

                    display();
                }
                
else
            /* PUT REQUEST */

                // input check
                if (strncmp(buffer, "put", 3) == 0){

                     if (strcmp(buffer, "put") == 0 || strcmp(buffer, "put ") == 0){
                        printf("Wrong synthax: please enter a file name.\n");
                        display();
                        continue;
                    }
                

                    cmd_put(socket_fd, buffer, server_addr, len);

                    // display available commands
                    display();
                }
else
            /* EXIT TRANSMIT */

                // input check
                if (strcmp(buffer, "exit") == 0){

                    // buffer sending
                    ret = sendto(socket_fd, buffer, BUFFER_SIZE, 0, (SA *) &server_addr, len);
                    // socket closing
                    close(socket_fd);
                    // check
                    if (ret == -1){
                        printf("sendto() error while sending exit command.\n"
                                "Client closing...\n");
                        close(socket_fd);
                        exit(-1);
                    }

                    exit(0);
                }
else
            /* INVALID INPUT */

                {
                    printf("Wrong synthax.\n");
                    display();
                }


        
        }
    
    return 0;
}
#include "../inc/interrupt.h"

extern int socket_fd;
extern struct sock_addr_in server_addr;
extern int len;

void SIGINT_handler(int signo, siginfo_t *a, void *b){

    int ret = sendto(socket_fd, "exit", sizeof("exit"), 0, (SA *) &server_addr, len);
    // socket closing
    close(socket_fd);
    if (ret == -1){
        printf("sendto() error while sending exit command after 'Ctrl + C'\n"
                "Client closing...\n");
        exit(-1);
    }


    printf("'exit' command sent succesfully.\n");

    // client closing
    exit(0);
}

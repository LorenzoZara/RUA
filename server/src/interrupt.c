#include "../inc/interrupt.h"
#include "../inc/config.h"

extern int socket_fd;
extern int num_client;
extern int free_port[MAX_CLIENTS];

void interrupt_handler(int signo, siginfo_t *a, void *b){
    close(socket_fd);
    exit(0);

}

void child_death_handler(int signo, siginfo_t *a, void *b){
    num_client--;
}

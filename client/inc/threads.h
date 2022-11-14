#ifndef _THREADS_H
#define _THREADS_H 

#include <pthread.h>
#include "../inc/config.h"
#include "../inc/libraries.h"

extern int num_pkt;
extern int semaphore;
extern int socket_fd;
extern struct sockaddr_in server_addr;
extern int len;

void *connection_checker(void *);
void *ack_receiver(void *);

#endif
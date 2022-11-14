#include "../inc/threads.h"
#include "../inc/libraries.h"
#include "../inc/types.h"
#include <semaphore.h> 

extern int semaphore;

void *connection_checker(void *arg){

    int *connected = (int *) arg;

    for (int i = TIMEOUT_CONNECTION; i > 0; i--){
        if (*connected) break;
    //    printf("%d seconds left for connection...\n", i);
        sleep(1);
    }

    if (!(*connected)) {
        printf("Couldn't connect to server.\n");
        exit(-1);
    }

}


void *ack_receiver(void *arg){
    
    /* THREAD STACK */

        int ret;
        int len = sizeof(server_addr);

        struct sembuf oper;

        int counter = 0;

        char buffer[BUFFER_SIZE];
        int seq;

        packet_man **pkts = (packet_man **) arg;

        struct timeval incoming;
        long time_incoming;

        pthread_t waiter_tid;
        int *connection = (int *) malloc(sizeof(int));
        *connection = 1;
        pthread_create(&waiter_tid, NULL, connection_checker, (void *) connection);

    /* HAZARD SECTION */

        while (1){

            oper.sem_num = 0;
            oper.sem_op = -1;
            oper.sem_flg = 0;

            semaphore_thread_commit:
            
            // trying to get permission to receive an ACK
            ret = semop(semaphore, &oper, 1);
            // check
            if (ret == -1){
                if (errno == EINTR) goto semaphore_thread_commit;
                else{
                    printf("semop() error in thread semaphore operations.\n");
                    printf("Client closing...\n");
                    close(socket_fd);
                    exit(-1);
                }
            }

            *connection = 0;

            // buffer flushing
            bzero(buffer, BUFFER_SIZE);
            // buffer receiving
            ret == recvfrom(socket_fd, buffer, BUFFER_SIZE, 0, (SA *) &server_addr, &len);   
            if (ret == -1){
                printf("recvfrom() error while receiving packet from server.\n");
                printf("Client closing...\n");
                close(socket_fd);
                exit(-1);
            }

            *connection = 1;

            seq = atoi(buffer);
 
            pkts[seq] -> ack ++;

            /*********************TIMEOUT_INTERVAL MANAGEMENT**********************/
            /**/    
            /**/    gettimeofday(&incoming, NULL);
            /**/    time_incoming = (((long)incoming.tv_sec)*1000000) + ((long)incoming.tv_usec);
            /**/
            /**/    pkts[seq] -> sampleRTT = time_incoming - (pkts[seq] -> time_start);
            /**/             
            /*********************************************************************/

            counter ++;
            if (counter == num_pkt) {
                pthread_exit(0);
            }

        }



}
#include "../inc/threads.h"
#include "../inc/libraries.h"
#include "../inc/types.h"
#include <semaphore.h>

extern int semaphore;

#include <pthread.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>


void *ack_receiver(void *arg){
    
    /* THREAD STACK */

        int ret;
        int len = sizeof(server_addr);

        struct sembuf oper;

        int counter = 0;

        char buffer[BUFFER_SIZE];
        int seq;

        packet_form **pkts = (packet_form **) arg;

        struct timeval incoming;
        long time_incoming;

    /* HAZARD SECTION */

        while (1){
            oper.sem_num = 0;
            oper.sem_op = -1;
            oper.sem_flg = 0;
semaphore_thread_commit:
            ret = semop(semaphore, &oper, 1);
            if (ret == -1){
                if (errno == EINTR) goto semaphore_thread_commit;
                else{
                    printf("semop() error in thread semaphore operations.\n");
                    printf("Client closing...\n");
                    close(socket_fd);
                    exit(-1);
                }
            }
            ret = recvfrom(socket_fd_child, buffer, 64, 0, (SA *) &client_addr, &len);   
            if (ret == -1){
                printf("recvfrom() error while receiving packet from server.\n");
                printf("Client closing...\n");
                close(socket_fd);
                exit(-1);
            }
            seq = atoi(buffer);
            pkts[seq]->ack++; 

            /*********************TIMEOUT_INTERVAL MANAGEMENT**********************/
            /**/    
            /**/    gettimeofday(&incoming, NULL);
            /**/    time_incoming = (((long)incoming.tv_sec)*1000000) + ((long)incoming.tv_usec);
            /**/
            /**/    pkts[seq] -> sampleRTT = time_incoming - (pkts[seq] -> time_start);
            /**/             
            /*********************************************************************/
            
            counter++;
            if (counter == num_pkt){
                pthread_exit(0);
            }
        }



}
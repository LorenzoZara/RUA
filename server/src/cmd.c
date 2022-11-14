#include "../inc/cmd.h"
#include "../inc/types.h"
#include "../inc/func.h"
#include "../inc/libraries.h"
#include "../inc/config.h" 
#include "../inc/threads.h"

#include <pthread.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>   

/*
int cmd_send_port(int socket_fd, struct sockaddr_in client_addr, int free_port[]);

    Description :   Calculates the free port to give for a new client

    Arguments :     - socket_fd : file descriptor of welcoming socket port
                    - client_addr : struct that contains client infos
                    - free_port : bitmap of used/unused ports

    Returns:        On success it returns a free port, otherwise -1 is returned 
*/
int cmd_send_port(int socket_fd, struct sockaddr_in client_addr, int *free_port){

    char buffer[BUFFER_SIZE];
    int client_port = 0;
    int len = sizeof(client_addr);          //                 0    1    2    3    4    5    7         MAX_CLIENTS
    for(int i = 1; i < MAX_CLIENTS; i++){   //free_port ---> |____|____|____|____|____|____|____|_..._|____|
        if (free_port[i] == 0){             //               1024 1025 1026 1027 1028 1029 1030       1024 + MAX_CLIENTS
            free_port[i] = 1;               //             welcoming
            client_port = PORT + i;         //              port
            break;
        }
    }
        if (client_port == 0){
            printf("Max number of client reached.\n");
            bzero(buffer, BUFFER_SIZE);
            sprintf(buffer, "%d", SERVICE_UNAVAILABLE);
            if (sendto(socket_fd, buffer, BUFFER_SIZE, 0, (SA *) &client_addr, len) == -1){
                printf("sendto() error %d: %s", SERVICE_UNAVAILABLE, strerror(errno)); 
                }
            return -1;
        }

        bzero(buffer,BUFFER_SIZE);
        sprintf(buffer,"%d",client_port);
        sendto(socket_fd,buffer,BUFFER_SIZE,0,(SA*) &client_addr,len);
        return client_port;
}

/*
void cmd_list(int socket_fd, struct sockaddr_in client_addr);

    Description:    send to client list of file in file_server directory

    Arguments:      - socket_fd : file descriptor of connection socket
                    - client_addr : struct that contains client infos

    Return values:  Returns void type in any case
*/
void cmd_list(int socket_fd, struct sockaddr_in client_addr){
    char buffer[BUFFER_SIZE];   
    int len=sizeof(client_addr);
    bzero(buffer,BUFFER_SIZE);
    if (strlen(dirfile()) <= 0) strcpy(buffer, "404 ");
    else{
        strcpy(buffer,"200 ");
        strcat(buffer, dirfile());
    }
    sendto(socket_fd, buffer, BUFFER_SIZE , 0, (SA *) &client_addr, len);
}

void cmd_corr(char * file_name, int socket_fd, struct sockaddr_in client_addr){
    int len=sizeof(client_addr);
    sendto(socket_fd, ispresent(file_name), 50 , 0, (SA *) &client_addr, len);
} 

int cmd_corr_put(char* file_size,int socket_fd, struct sockaddr_in client_addr){
    int len=sizeof(client_addr);
    if (strtol(file_size, NULL, 10) > MAX_FILE_SIZE){
        sendto(socket_fd, "406", 3 , 0, (SA *) &client_addr, len);
        return 0;
    } else if( atoi(file_size) == 0){
        sendto(socket_fd, "406", 3 , 0, (SA *) &client_addr, len);
        return 0;
    }
    else{
        sendto(socket_fd, "200", 3 , 0, (SA *) &client_addr, len);
        return 1;
    }
}

int semaphore;

/*
void cmd_send_packets(int socket_fd, char *buffer, struct sockaddr_in client_addr, int len);

    Description :       This function's goal is to manage the receiving packets phase
                        while server is uploading file to client

    Arguments   :       - socket_fd : file descriptor of the socket used by connection
                        - buffer : address of the buffer used to send/receive
                        - client_addr : struct that contains client infos
                        - len : client address length

    Return values:      Returns void type in any case           
*/
void cmd_send_packets(int socket_fd, char *buffer, struct sockaddr_in client_addr, int len){
    
    // checking values variable
    int ret;

    // infos on uploading file
    char filename[128];
    int file_len; char str_file_len[17]; // 10 cifre circa 1 GB
    char path[200] = DIRECTORY;
    int temp_fd;
    char packetToSend[BUFFER_SIZE];
    // code status from server variables 
    char str_response[4]; int response;

    // adaptive timer variables
    long estimatedRTT = 0, devRTT = 0, timeout_interval = TIMEOUT_INTERVAL;

    // packets variables

    // threadsf
    pthread_t ack_controller_tid, ack_receiver_tid;

    // semaphore buffer
    struct sembuf oper; 

    snprintf(path, 200, "%s%s", DIRECTORY, buffer);
	//strcat(path, filename);

printf("Pathname is %s\n", path);
    // check if file exists
    int exists = 1;

    // open file to get length
    temp_fd = open(path, O_RDONLY, 0666);

    // check
    if (temp_fd == -1){
        printf("File %s is not stored in this server. Sending NOT_FOUND message.\n", path);
        exists = 0;
    }
    


    // If file doesn't exists, send error
    if (!exists){
        bzero(str_file_len, sizeof(str_file_len));
        snprintf(buffer, 4, "%d", 404);
        ret = sendto(socket_fd, buffer, BUFFER_SIZE, 0, (SA *) &client_addr, len);
        return;
    }

    // get file length
    file_len = lseek(temp_fd, 0, SEEK_END);
    close(temp_fd);
    bzero(packetToSend, BUFFER_SIZE);
    snprintf(packetToSend, BUFFER_SIZE, "%d %s %d", OK, buffer, file_len);

    // If file size is !>0, send error
    if (!(file_len > 0)){
        snprintf(buffer, 4, "%d", 404);
        ret = sendto(socket_fd, buffer, BUFFER_SIZE, 0, (SA *) &client_addr, len);
        return;
    }

    // buffer sending
    ret = sendto(socket_fd, packetToSend, BUFFER_SIZE, 0, (SA *) &client_addr, len);
    // check
    if (ret == -1){
        printf("sendto() error while sending put command.\n"
                "Client closing...\n");
        close(socket_fd);
        exit(-1);
    }

    // calculate number of packets needed
    int num_pkt = ceilf((float)file_len / (float)PAYLOAD);
    packet_form **pkts;
    printf("@@@ file_len = %d\n    payload = %d\n    num_pkt = ceil(%d/%d) = %d\n", file_len, PAYLOAD, file_len, PAYLOAD, num_pkt);

    // open file to copy bytes in payload
    temp_fd = open(path, O_RDONLY, 0666);
    if (temp_fd == -1){
        printf("open error while opening file for packets building.\n");
        printf("Closing client...\n");
        close(socket_fd);
        exit(-1);
    }

    // allocating memory for management struct of packets
    pkts = (packet_form **) malloc(num_pkt * sizeof(packet_form *));
    if (pkts == NULL){
        printf("malloc error while allocating management structs for packets.\n");
        printf("Client closing.\n");
        close(socket_fd);
        exit(-1);
    }
    // initializzating structs
    for (int i=0; i<num_pkt; i++){
        pkts[i] = (packet_form *) malloc(sizeof(packet_form));
        // check
            if (pkts[i] == NULL){
                printf("malloc error while initializzating structs.\n");
                printf("Client disconnection.\n");
                close(socket_fd_child);
                exit(-1);
            }

        pkts[i]->seq = i;
        read(temp_fd, pkts[i] -> payload, PAYLOAD);    
        pkts[i]->ack = 0;
        pkts[i]->checked = 0;
        pkts[i]->sent = 0;

    }   

    // closing I/O temporarily session
    close(temp_fd);

    printf("All packets (%d) have been built and file is closed.\n", num_pkt);

    // initialize semaphore for ack_receiver thread
    semaphore = semget(IPC_PRIVATE, 1, 0666);
    ret = semctl(semaphore, 0, SETVAL, 0);

    // main_thread routin : sending packets
        
        pthread_create(&ack_receiver_tid, NULL, ack_receiver, (void *) pkts);

        // base number of transmission window
        int base = 0;

        char data[BUFFER_SIZE];
        int counterACK = 0;
        int rtx_pkts = 0;

        int window = WINDOW;
        if (num_pkt < window) window = num_pkt;

        struct timeval final, eof;
        long time_start, time_stamp, time_eof, download_time;

        printf("Sending packets...\n");


        //                                   base          (num_pkt-TX_WINDOW)    num_pkt
        while (counterACK < num_pkt){ //      |_|_|_|_|_|_|_|_|_|_|_|_|_|_|_|_|_|_|_|
//printf("@@@ Number of ACK received %d.\n", counterACK);
            for(int i=base; i<base+window; i++){ //   base    i       base+TX_WINDOW
                                                    //    |_|_|_|_|_|_|_|_|_|_|
//if (counterACK < num_pkt){
                if (!(pkts[i]->sent)){

                    bzero(data, BUFFER_SIZE);
                    snprintf(data, SEQ_SIZE, "%d", pkts[i] -> seq);
                    memcpy(data + SEQ_SIZE, pkts[i] -> payload, PAYLOAD);

                    ret = sendto(socket_fd, data, BUFFER_SIZE, 0, (SA *) &client_addr, len);                   
                        // check 
                    if (ret == -1){
                        printf("sendto() error while sending list command.\n");
                        close(socket_fd);
                        exit(-1);
                    }
                   
                    pkts[i]->sent = 1;
    //                printf("@@@ Pkt %d transmitted.\n", i);

                    // setting pkt starting time    (measure unit is microsec)
                    gettimeofday(&(pkts[i]->initial), NULL);
                    pkts[i]->time_start = (((long)((pkts[i] -> initial).tv_sec))*1000000) +                          
                                                (long)((pkts[i] -> initial).tv_usec);

                    if (i==0) time_start = pkts[i] -> time_start;
                     
                    oper.sem_num = 0;
                    oper.sem_op = 1;
                    oper.sem_flg = 0;

                    semaphore_commit:   // give ACK receiver thread a chance to receive an ack

                    ret = semop(semaphore, &oper, 1);
                    if ( ret == -1 ){
                        if (errno == EINTR) goto semaphore_commit;
                        else{
                            printf("semop() error in semaphore operations.\n");
                            printf("Client closing...\n");
                            close(socket_fd);
                            exit(-1);
                        }
                    }

                }

                // if main_thread receives ACK of base window packet then transmission window will slide
                if (pkts[i]->ack){
                    if (!(pkts[i]->checked)){
    //                    printf("ACK %d is checked.\n", i);
                        pkts[i]->checked++;
                        counterACK++;

                        /* *** ADAPTIVE TIMEOUT INTERVAL *** */
 
                            estimatedRTT = (((long)((double)1 - ALPHA))*estimatedRTT) + (((long)ALPHA) * (pkts[i]->sampleRTT));

                            devRTT = (((long)(((double)1) - BETA))*devRTT) + (((long)BETA)*abs((pkts[i]->sampleRTT)-estimatedRTT));


                            if (ADAPTIVE_TIMER)
                                timeout_interval = estimatedRTT + (((long)4) * devRTT);

                        /**************************************/

                    }
                    if ((pkts[i]->seq == base) && (base < num_pkt - window)){
                        base ++;
                        free(pkts[i]);
                    }
                } else {// timemout interval check


                    //actual timestamp
                    gettimeofday(&final, NULL);
                    time_stamp = ((long)(final.tv_sec) * 1000000) + (long)(final.tv_usec);
 
                  //if (initial timestamp) + timeout > (actual timestamp)
                    long diff = time_stamp - (pkts[i]->time_start);

                    if (diff > timeout_interval){  
     
                            if (ADAPTIVE_TIMER)
                                timeout_interval = timeout_interval * 2;


                        // RETRANSMISSION

                        rtx_pkts ++;

                        bzero(data, BUFFER_SIZE);
                        snprintf(data, SEQ_SIZE, "%d", pkts[i] -> seq);
                        memcpy(data + SEQ_SIZE, pkts[i] -> payload, PAYLOAD);

                        sendto(socket_fd, data, BUFFER_SIZE, 0, (SA *) &client_addr, len);
    //                    if (!(pkts[i] -> checked)) printf("Pkt %d is retransmitted.\n", i);
                        
                        
                        // setting pkt starting time    (measured in microsec)
                        gettimeofday(&(pkts[i] -> initial), NULL);
                        pkts[i]->time_start = (((double)((pkts[i] -> initial).tv_sec))*1000000) +                          
                                                (double)((pkts[i] -> initial).tv_usec);

                        if (i==0) time_start = pkts[i] -> time_start;

                    }    
                } 
            }            
        } 

        free(pkts);

        /* FILE RECEIVED INFOS */
 
            gettimeofday(&eof, NULL);
            time_eof = (((long) eof.tv_sec)*1000000) + ((long) eof.tv_usec);

            download_time = time_eof - time_start;

            double download_time_sec = ((double)download_time)/((double)1000000);

            //printf("Packets lost    :   %d\n", rtx_pkts);
            printf("DOWNLOAD TIME   :   %f seconds\n", download_time_sec);
            printf("THROUGHPUT      :   %f Mbps ", ((((double)8)*((double)file_len ))/ download_time));
            printf("[ %f  pkts / sec ]\n", ((double) num_pkt) / download_time_sec);
    
}

  



void cmd_recv_packets(char* file_name,char* file_size ,int socket_fd, struct sockaddr_in client_addr){
    
    int ret;
    srand(time(NULL));

    // buffer initializzation
    char buffer[BUFFER_SIZE];
    //buffer flushing
    bzero(buffer,sizeof(buffer));

    // seq initializzation
    char seq[SEQ_SIZE];
    // seq flushing
    bzero(seq,sizeof(seq));

    int len=sizeof(client_addr);
    
    // redirecting upload to file_server directory
    strcpy(buffer,DIRECTORY);
    strcat(buffer,file_name);

    // creating file to write (truncate it if it already exists)
    int fd = open(buffer, O_CREAT | O_RDWR | O_TRUNC, 0666);
    // check
        if (fd == -1){
            printf("Server couldn't open %s.\n", buffer);
        }

    // calculate number of packets to be received    
    int num_pkt=ceilf((float)(atoi(file_size)/(float)PAYLOAD));

    // receive window initializzation
    int base=0;
    int counter=0;

    // initializzating management struct of packets
    packet_form ** packet= (packet_form **) malloc(num_pkt * sizeof(packet_form *));
    if (packet == NULL) {
        printf("malloc error while creating management structs.\n");
        printf("Server is shutting down...\n");
        exit(-1);
    }

    for (int i = 0; i < num_pkt; i++){
        packet[i] = (packet_form *) malloc(sizeof(packet_form));
        if (packet[i] == NULL){
            printf("malloc error while creating management struct for packet %d.\n", i);
            printf("Server is shutting down...\n");
            exit(-1);
        }

        packet[i] -> seq = i;
        packet[i] -> recv = 0;
        packet[i] -> checked = 0;
    }
    printf("Number of packets: %d\n", num_pkt);

    int window=WINDOW;
    // if number of packets is less than window size, than window size = number of packets
    if(num_pkt<window){
        window=num_pkt;
    }

    // while all ACKs haven't been received
    while(counter < num_pkt){       rec:


            if (counter == num_pkt) goto write_payload;

            // buffer flushing
            bzero(buffer, sizeof(buffer));
            // buffer receiving
            recvfrom(socket_fd, buffer, sizeof(buffer), 0, (SA *) &client_addr, &len);

            // getting sequence number of packet
            strncpy(seq, buffer, SEQ_SIZE);

            // packet loss simulation
            if(prob(PROBABILITY_LOSS)){
    //            printf("Packet %d has been lost. (simulation)\n",atoi(seq));
                goto rec;
            } 

            // packet discarded cause sequence number is out of window
            if(atoi(seq) > window + base){
    //            printf("Packet %d has been discarted. (out of window)\n", atoi(seq));
                goto rec;
            }

            // packet discarded cause already captured it
            if(packet[atoi(seq)] -> recv){
    //            printf("Packet %d has been discarted. (already captured it)\n", atoi(seq));
                goto rec;
            }

    //        printf("Packet %d captured.\n", atoi(seq));

            packet[atoi(seq)] -> recv ++;
            packet[atoi(seq)] -> seq = atoi(seq);
            counter ++;

            // ack sending
            sendto(socket_fd, seq, sizeof(seq), 0, (SA *) &client_addr, len);   
            if (atoi(seq) + 1 == num_pkt){  //if is the last packet
                memcpy(packet[atoi(seq)] -> payload, buffer + SEQ_SIZE, atoi(file_size)-(packet[atoi(seq)] -> seq * PAYLOAD)); 
            }else{
                memcpy(packet[atoi(seq)] -> payload, buffer + SEQ_SIZE, PAYLOAD);            
            }
            
            write_payload: 


            for(int i=base; i<base+window; i++){
                
                if ((packet[i] -> recv) && (i == base)){ 
                    if (packet[i] -> seq + 1 == num_pkt){      // if is the last packet
                        write(fd, packet[i] -> payload, atoi(file_size)-(packet[atoi(seq)] -> seq*PAYLOAD));
                    }else{
                        write(fd, packet[i] -> payload, PAYLOAD);
                    }

                    base ++;
                    free(packet[i]);
                    if (base > num_pkt - window) window --;
                } 
            }
        
    //    printf("%d packets received succesfully.\n", base);
    }

    free(packet);

    printf("%d packets received succesfully.\n", num_pkt);
    printf("File received correctly.\n");
    close(fd);
}


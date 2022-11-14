#include "../inc/cmd.h"
#include "../inc/threads.h" 

#include <time.h>
#include <sys/time.h>
#include <pthread.h>

int num_pkt; 

/* 

void cmd_recv_packets(char* file_name,char* file_size ,int socket_fd, struct sockaddr_in server_addr); 

    Description:    This function's goal is to manage the receiving packets phase
                    while client is downloading file from server

    Arguments:      - file_name : name of the file to download
                    - file_size : length of the file to download
                    - socket_fd : file descriptor of the socket used by connection
                    - server_addr : struct that contains server address infos

    Return values:  Returns void type in any case
*/
void cmd_recv_packets(char* file_name,char* file_size ,int socket_fd, struct sockaddr_in server_addr){

    int ret;    
    srand(time(NULL));
    
    // buffer initializzation
    char buffer[BUFFER_SIZE];
    // buffer flushing
    bzero(buffer, BUFFER_SIZE);
    
    // seq initializzation
    char seq[SEQ_SIZE];
    // seq flushing
    bzero(seq, SEQ_SIZE);

    int len = sizeof(server_addr);

    // redirecting downloading to file_client directory
    strcpy(buffer, DIRECTORY);
    strcat(buffer, file_name);

    // creating file to write (truncate it if it already exists)    
    int fd = open(buffer, O_CREAT | O_RDWR | O_TRUNC, 0666);
    // check
        if (fd == -1){
            printf("Server couldn't open %s.\n", buffer);
        }

    // calculate number of packets to be received
    int num_pkt = ceilf((float) (atoi(file_size) / (float) PAYLOAD)); 

    // receive window initializzating
    int base=0;
    int counter=0; 

    // initializzating management struct of packets
    packet_man ** packet = (packet_man **) malloc(num_pkt * sizeof(packet_man *));
    if (packet == NULL){
        printf("malloc error while creating management structs.\n");
        printf("Closing client...\n");
        exit(-1);
    }

    for (int i = 0; i < num_pkt; i++){
        packet[i] = (packet_man *) malloc(sizeof(packet_man));
        if (packet[i] == NULL){
            printf("malloc error while creating management struct of packet %d.\n", i);
            printf("Client closing...\n");
            exit(-1);
        }

            packet[i] -> seq = i;
            packet[i] -> ack = 0;
            packet[i] -> ack_checked = 0;
    }
    printf("Number of packets: %d\n", num_pkt);

    int window = WINDOW;
    // if number of packets is less than window size, than window size = number of packets
        if (num_pkt < window){
            window=num_pkt;
        }

    int *connection = (int *) malloc(sizeof(int));
    *connection = 1;

    pthread_t waiter_tid;
    pthread_create(&waiter_tid, NULL, connection_checker, (void *) connection);

    // while all ACKs haven't been received
    while (counter < num_pkt){      rec:

            if (counter == num_pkt) goto write_payload;
            
            *connection = 0;
            // buffer flushing
            bzero(buffer, BUFFER_SIZE);
            // buffer receiving
            ret = recvfrom(socket_fd, buffer, sizeof(buffer), 0, (SA *) &server_addr, &len);
            // check
                if (ret == -1){
                    printf("recvfrom() error while receiving packets from server.\n");
                    printf("Closing client...\n");
                    close(fd); close(socket_fd);
                    exit(-1);
                }
            *connection = 1;

            // getting sequence number of packet
            strncpy(seq,buffer,SEQ_SIZE);

            // packet loss simulation
            if(prob(PROBABILITY_LOSS)){
    //            printf("Packet %d has been lost. (simulation)\n",atoi(seq));
                goto rec;
            }  

            // packet discarded cause number of sequence is out of window
            if(atoi(seq) > window + base){
    //            printf("Packet %d has been discarded. (out of window)\n", atoi(seq));
                goto rec;
            }

            // packet discarded cause already captured it
            if(packet[atoi(seq)] -> ack){
    //            printf("Packet %d has been discarded. (already captured it)\n", atoi(seq));
                goto rec;
            }

    //        printf("Packet %d captured.\n", atoi(seq));

            packet[atoi(seq)] -> ack ++;
            packet[atoi(seq)] -> seq = atoi(seq);
            counter ++;

            // ack sending
            ret = sendto(socket_fd, seq, sizeof(seq), 0, (SA *) &server_addr, len);
            // check
                if (ret == -1){
                    printf("sendto() error while receiving packet from server.\n");
                    printf("Client closing...\n");
                    close(socket_fd);
                    exit(-1);
                }

            //if is the last packet than copy 1024 bytes in buffer, else the remaining ones until the end of file
            if (atoi(seq) + 1 == num_pkt){  
                bcopy(buffer + SEQ_SIZE, packet[atoi(seq)] -> payload, atoi(file_size)-(packet[atoi(seq)] -> seq*PAYLOAD));
            }else{
                bcopy(buffer + SEQ_SIZE, packet[atoi(seq)] -> payload, PAYLOAD);
            }   
                               
            write_payload:

            // scroll in-window received packets to write right* payload on file    [*if sequence number == base]
            for (int i = base; i < base + window; i++){

                if ((packet[i] -> ack) && (i == base)){ 

                    if (packet[i] -> seq + 1 == num_pkt){      // if is the last packet write remaining bytes...
                        write(fd, packet[i] -> payload, atoi(file_size)-(packet[i] -> seq*PAYLOAD)), atoi(file_size)-((counter)*PAYLOAD);
                    }else{                                  // else write 1024 bytes
                        write(fd, packet[i] -> payload, PAYLOAD);
                    }
                    // let the window slide
                    base ++;
                    free(packet[i]);
                    // if the upper bound of window reach number of packets than reduce window size
                    if (base > num_pkt - window) window --;
                } 
            }
        
    //    printf("%d packets received succesfully.\n", base);

        // buffer flushing
        bzero(buffer,BUFFER_SIZE);
    }

    free(packet);

    printf("%d packets received succesfully.\n", num_pkt);
    printf("File received correctly.\n");

    close(fd);
}

/* 

void cmd_list(int socket_fd, char *buffer, struct sockaddr_in server_addr, int len);

    Description:    This function goals is to manage the receiving phase while client
                    is waiting for the list of files contained in server

    Arguments:      - socket_fd : file descriptor of the socket used by connection
                    - buffer : address of the buffer used to send/receive
                    - server_addr : struct that contains server address infos
                    - len : server address length

    Return values:  Returns void type in any case
*/
void cmd_list(int socket_fd, char *buffer, struct sockaddr_in server_addr, int len){

    // checking values variable
    int ret;

    // status code from server
    char str_response[4]; int response;

    // string of server available files
    char file_list[BUFFER_SIZE];

    // buffer sending
    ret = sendto(socket_fd, buffer, BUFFER_SIZE, 0, (SA *) &server_addr, len);
    // check 
        if (ret == -1){
            printf("sendto() error while sending list command.\n");
            close(socket_fd);
            exit(-1);
        }

    retry_list_receiving:

    // buffer flushing
    bzero(buffer, BUFFER_SIZE);
    // buffer receiving : receiving list of available files on server
    //          |
    //          |----> EXPECTED FORMAT OK:                           "200 file_list"
    //          |----> UNEXPECTED FORMAT BAD REQUEST:                "400"
    //          |----> UNEXPECTED FORMAT SERVICE UNAVAILABLE:        "503"
    ret = recvfrom(socket_fd, buffer, BUFFER_SIZE, 0, (SA *) &server_addr, &len);
    // check
        if (ret == -1){
            printf("recvfrom() error while receiving file list.\n");
            close(socket_fd);
            exit(-1);
        }

    // getting status code from buffer
    strcpy(str_response, strtok(buffer, " "));

    display_response_message(str_response, "\0", "\0");

    

    // handling cases in which server doesn't confirm the operation (status cose != OK)
    response = atoi(buffer);

    if (response == SERVICE_UNAVAILABLE){
        printf("Server has shutdown.\n");
        close(socket_fd);
        exit(-1);
    }

    else if (response == BAD_REQUEST){
        printf("Unknown command.\n");
        return;

    }

    else if (response == NOT_FOUND){
        printf("No files stored in server.\n");
        return;
    }
    
    else if (response != OK){
        goto retry_list_receiving;
    }

    // getting file_list from buffer
    strcpy(file_list, strtok(NULL, "\0"));

    printf("\nLIST OF SERVER AVAILABLE FILES:\n%s\n\n", file_list);

}

// semaphore shared with receiving ACK thread
int semaphore;

/*

int cmd_put(int socket_fd, char *buffer, struct sockaddr_in server_addr, int len){

    Description:    socket_fd : file descriptor of the socket used by connection
                    buffer : address of buffer used to send/receive
                    server_addr : struct that contains server address infos
                    len : server address length

    Return values : Returns void type in any case 
*/
void cmd_put(int socket_fd, char *buffer, struct sockaddr_in server_addr, int len){

    // adaptive timer variables
    long estimatedRTT = 0, devRTT = 0;
    
    // timeout interval variable
    long timeout_interval = TIMEOUT_INTERVAL;
 
    // checking values variable
    int ret;

    // infos on uploading file
    char filename[FILE_NAME_SIZE];
    int file_len; char str_file_len[11]; // 10 cifre circa 10 GB
    char path[PATH_NAME_SIZE] = DIRECTORY;
    int temp_fd;

    // code status from server variables
    char str_response[4]; int response;

    // packets variables 
    packet_man **pkts;

    // threads
    pthread_t ack_controller_tid, ack_receiver_tid;

    // semaphore buffer
    struct sembuf oper;

    strtok(buffer, " ");
    // get filename from user input
	strcpy(filename, strtok(NULL, "\0"));
    // get pathname
    snprintf(path, 200, "%s%s", DIRECTORY, filename);

    printf("Uploading %s\n", path);

    // open file to get length
    temp_fd = open(path, O_RDONLY, 0666);
    // check
        if (temp_fd == -1){
            printf("Couldn't open %s. File is corrupted or it doesn't exists.\n", path);
            return;
        }

    // get file length
    file_len = lseek(temp_fd, 0, SEEK_END);
    close(temp_fd);

    // str_file_len flushing
    bzero(str_file_len, sizeof(str_file_len));
    sprintf(str_file_len, "%d", file_len);

    // buffer rebuilt, actual value = "put"
    strcat(buffer, " ");
    // buffer rebuilt, actual value = "put " 
    strcat(buffer, filename);
    // buffer rebuilt, actual value = "put file_name"
    strcat(buffer, " ");
    // buffer rebuilt, actual value = "put file_name "
    strcat(buffer, str_file_len);
    // buffer rebuilt, actual value = "put file_name file_len"

    display_request_message("put", filename, str_file_len);

    // buffer sending
    ret = sendto(socket_fd, buffer, BUFFER_SIZE, 0, (SA *) &server_addr, len);
    // check
        if (ret == -1){
            printf("sendto() error while sending put command.\n"
                    "Client closing...\n");
            close(socket_fd);
            exit(-1);
        }

    // buffer flushing
    bzero(buffer, BUFFER_SIZE);                         retryReceive:
    // buffer receiving : receiving put permission
    //          |-------------> EXPECTED FORMAT OK:                     "200"
    //          |-------------> UNEXPECTED FORMAT BAD REQUEST:          "400"
    //          |-------------> UNEXPECTED FORMAT NOT ACCEPTABLE:       "406"
    //          |-------------> UNEXPECTED FORMAT SERVICE UNAVAILABLE:  "503"
    ret = recvfrom(socket_fd, buffer, BUFFER_SIZE, 0, (SA *) &server_addr, &len);
    // check
        if (ret == -1){
            printf("recvfrom() error while receiving put permission.\n"
                    "Client closing...\n");
            close(socket_fd);
            exit(-1);
        }


    // status code value
    response = atoi(buffer);
    // checking...
    if (response != OK){

        if (response == BAD_REQUEST){ 
            display_response_message(buffer, "\0", "\0");
            printf("Unknown command.\n");
            return;
        }

        else if (response == NOT_ACCEPTABLE){
            display_response_message(buffer, "\0", "\0");
            printf("You can not upload your file.\n");
            return;
        }
        
        else if (response == SERVICE_UNAVAILABLE){
            printf("Server has shutdown.\n"
                    "Client closing...\n");
            close(socket_fd);
        }
        
        else goto retryReceive;
    }
    
    display_response_message(buffer, "\0", "\0");

    // calculate number of packets needed
    num_pkt = ceilf((float)file_len / (float)PAYLOAD);
    printf("%s size is %d. Splitting data in %d packets.\n", filename, file_len, num_pkt);

    // open file to copy bytes in payload
    temp_fd = open(path, O_RDONLY, 0666);
    // check
        if (temp_fd == -1){
            printf("open error while opening file for packets building.\n");
            printf("Closing client...\n");
            close(socket_fd);
            exit(-1);
        }

    // memory allocation for packets structs
    pkts = (packet_man **) malloc(num_pkt * sizeof(packet_man *));
    if (pkts == NULL){
        printf("malloc error while allocating management structs for packets.\n");
        printf("Client closing.\n");
        close(socket_fd);
        exit(-1);
    }
    for (int i=0; i<num_pkt; i++){
        pkts[i] = (packet_man *) malloc(sizeof(packet_man));
        if (pkts[i] == NULL){
            printf("malloc error while allocating management struct %d.\n", i);
            printf("Client closing.\n");
            close(socket_fd);
            exit(-1);
        }
        // packet struct building
        pkts[i] -> seq = i;
        pkts[i] -> sent = 0;
        read(temp_fd, pkts[i] -> payload, PAYLOAD);    
        pkts[i] -> ack = 0;
        pkts[i] -> ack_checked = 0;
    }

    // closing I/O temporarily session
    close(temp_fd);

printf("@@@ All packets (%d) have been built and file is closed.\n", num_pkt);

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

        struct timeval final;
        long time_stamp;

        struct timeval eof;
        long time_start, time_eof, upload_time;

        printf("Sending packets...\n");


        //                                   base          (num_pkt-TX_WINDOW)    num_pkt
        while (counterACK < num_pkt){ //      |_|_|_|_|_|_|_|_|_|_|_|_|_|_|_|_|_|_|_|


            for(int i=base; i<base+window; i++){ //   <-base->    i     <-base+TX_WINDOW->
                                                    //    |_|_|_|_|_|_|_|_|_|_|

                if (!(pkts[i] -> sent)){

                    bzero(data, BUFFER_SIZE);
                    snprintf(data, SEQ_SIZE, "%d", pkts[i] -> seq);
                    memcpy(data + SEQ_SIZE, pkts[i] -> payload, PAYLOAD);

                    sendto(socket_fd, data, BUFFER_SIZE, 0, (SA *) &server_addr, len);
                    
                    pkts[i] -> sent = 1;
    //                printf("Pkt %d is transmitted.\n", i);

                    // setting pkt starting time    (measure unit is microsec)
                    gettimeofday(&(pkts[i] -> initial), NULL);
                    pkts[i] -> time_start = (((long)((pkts[i] -> initial).tv_sec))*1000000) +                          
                                                (long)((pkts[i] -> initial).tv_usec);

                    if (i == 0) time_start = pkts[i] -> time_start;
                    
                    // give the thread a chance to receive an ack
                    oper.sem_num = 0;
                    oper.sem_op = 1;
                    oper.sem_flg = 0;
semaphore_commit_1:
                    ret = semop(semaphore, &oper, 1);
                    if ( ret == -1 ){
                        if (errno == EINTR) goto semaphore_commit_1;
                        else{
                            printf("semop() error in semaphore operations.\n");
                            printf("Client closing...\n");
                            close(socket_fd);
                            exit(-1);
                        }
                    }

                }

                // if main_thread checks the ACK of base window packet then transmission window will slide
                if (pkts[i] -> ack){
                    if (!(pkts[i] -> ack_checked)){
    //                    printf("ACK %d is checked.\n", i);
                        pkts[i] -> ack_checked ++;
                        counterACK ++;

                            /* *** ADAPTIVE TIMEOUT INTERVAL SETTINGS *** */

                            estimatedRTT = (((long)((double)1 - ALPHA))*estimatedRTT) + 
                                            (((long)ALPHA) * (pkts[i]->sampleRTT));

                            devRTT = (((long)(((double)1) - BETA))*devRTT) + 
                                        (((long)BETA)*abs((pkts[i]->sampleRTT)-estimatedRTT));

                            if (ADAPTIVE_TIMER) 
                                timeout_interval = estimatedRTT + (((long)4) * devRTT);

                            /***********************************************/

                    }
                    if ((pkts[i] -> seq == base) && (base < num_pkt - window)){
                        base ++;
                        free(pkts[i]);
                    }
                } else {// timemout interval check

                    //actual timestamp
                    gettimeofday(&final, NULL);
                    time_stamp = ((long)(final.tv_sec) * 1000000) + (long)(final.tv_usec);
 
                    long diff = time_stamp - (pkts[i] -> time_start);
                
                    // if transmission deadline is reached (timeout)
                    if (diff > timeout_interval){  

                        // RETRANSMISSION 

                        rtx_pkts ++;

                        bzero(data, BUFFER_SIZE);
                        snprintf(data, SEQ_SIZE, "%d", pkts[i] -> seq);
                        memcpy(data + SEQ_SIZE, pkts[i] -> payload, PAYLOAD);

                        ret = sendto(socket_fd, data, BUFFER_SIZE, 0, (SA *) &server_addr, len);
                        // check
                        if (ret == -1){
                            printf("sendto() error while retransmitting a packet.\n");
                            printf("Client closing...\n");
                            close(socket_fd);
                            exit(-1);
                        }
    //                    printf("Pkt %d is retransmitted.\n", i);
                        
                        if (ADAPTIVE_TIMER)
                            timeout_interval = timeout_interval * 2;

                        
                        // setting pkt starting time    (measure unit is microsec)
                        gettimeofday(&(pkts[i] -> initial), NULL);
                        pkts[i] -> time_start = (((double)((pkts[i] -> initial).tv_sec))*1000000) +                          
                                                (double)((pkts[i] -> initial).tv_usec);

                        if (i == 0) time_start = pkts[i] -> time_start;

                    } 
                }
            }             
        } 

        free(pkts);   
 
        /* FILE SENT INFOS */
 
            gettimeofday(&eof, NULL);
            time_eof = (((long) eof.tv_sec)*1000000) + ((long) eof.tv_usec);

            upload_time = time_eof - time_start;

            double upload_time_sec = ((double)upload_time)/((double)1000000);

            //printf("Packets lost    :   %d\n", rtx_pkts);
            printf("UPLOAD TIME :   %f seconds\n", upload_time_sec);
            printf("THROUGHPUT  :   %f Mbps ", ((((double)8) * (double)file_len) )/ upload_time);
            printf("[ %f pkts / sec ]\n", ((double)num_pkt) / upload_time_sec);

} 
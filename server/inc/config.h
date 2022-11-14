#ifndef _CONFIG_H
#define _CONFIG_H

    /* *** CONFIGURATION PARAMETERS *** */

    #define TIMEOUT_INTERVAL 1000000 // microseconds          --> 1 sec
    #define ADAPTIVE_TIMER 1
    #define ALPHA 0.125
    #define BETA 0.25
    #define MAX_CLIENTS 10
    #define MAX_FILE_SIZE 2147483648 // bytes                 --> 2 GB
    #define WINDOW 8
    #define PROBABILITY_LOSS 30

    /* ******************************** */

#define DIRECTORY "./file_server/"
#define PORT 1024
#define SA struct sockaddr

#define OK 200
#define BAD_REQUEST 400
#define NOT_FOUND 404
#define NOT_ACCEPTABLE 406
#define SERVICE_UNAVAILABLE 503

#define SEQ_SIZE 64
#define PAYLOAD 1024
#define BUFFER_SIZE 1088

#endif
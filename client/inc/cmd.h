#ifndef CMD_H
#define CMD_H

#include "../inc/libraries.h"
#include "../inc/config.h"
#include "../inc/func.h"
#include "../inc/types.h"
#include <pthread.h>
#include <time.h>
#include <sys/time.h>

//void cmd_list(int socket_fd, struct sockaddr_in client_addr);
void cmd_corr(char * file_name, int socket_fd, struct sockaddr_in client_addr);
void cmd_corr_put(char* file_size,int socket_fd, struct sockaddr_in client_addr);
//void cmd_send_packets(char* file_name,int socket_fd, struct sockaddr_in client_addr);
void cmd_list(int socket_fd, char *buffer, struct sockaddr_in server_addr, int len);
void cmd_put(int socket_fd, char *buffer, struct sockaddr_in server_addr, int len);
void cmd_recv_packets(char* file_name,char* file_size ,int socket_fd, struct sockaddr_in server_addr);

#endif
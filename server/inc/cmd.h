#ifndef CMD_H
#define CMD_H

#include "../inc/libraries.h"
#include "../inc/config.h"
#include "../inc/func.h"

int cmd_send_port(int socket_fd, struct sockaddr_in client_addr, int free_port[]);
void cmd_list(int socket_fd, struct sockaddr_in client_addr);
void cmd_corr(char * file_name, int socket_fd, struct sockaddr_in client_addr);
int cmd_corr_put(char* file_size,int socket_fd, struct sockaddr_in client_addr);
void cmd_recv_packets(char* file_name,char* file_size,int socket_fd, struct sockaddr_in client_addr);
void cmd_send_packets(int socket_fd, char *buffer, struct sockaddr_in client_addr, int len);
#endif
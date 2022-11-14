#ifndef FUNC_H
#define FUNC_H

#include "../inc/libraries.h"
#include"../inc/config.h"


int create_socket(int s_port, struct sockaddr_in server_addr);
char* ispresent(char* file_name);
void display();
void display_request_message(char *, char *, char *);
void display_response_message(char *, char *, char *);
int prob_perdita( int prob);
int prob(int prob);
int checkReply(char* buffer);


#endif
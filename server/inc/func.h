#ifndef FUNC_H
#define FUNC_H

#include "../inc/libraries.h"
#include"../inc/config.h"


int create_socket(int s_port);
char* dirfile();
char* ispresent(char* file_name);
int prob(int prob);
void *ack_receiver(void *arg);

#endif
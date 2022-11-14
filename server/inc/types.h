#ifndef _TYPES_H
#define _TYPES_H

#include "config.h"

typedef struct _packet_form{
	int seq;

	char payload[PAYLOAD];

	int recv;

	int checked;

	int ack;

	int sent;

	long sampleRTT;

		//timestamp
	struct timeval initial;
	long time_start;
} packet_form;


struct sockaddr_in server_addr;
struct sockaddr_in  client_addr;
int num_pkt;
int socket_fd_child;

#endif
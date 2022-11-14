#ifndef INTERRUPT_H
#define INTERRUPT_H

#include "../inc/libraries.h"
#include "../inc/config.h"

void SIGINT_handler(int signo, siginfo_t *a, void *b);
/*
void interrupt_handler(int, siginfo_t *, void *);
void child_death_handler(int, siginfo_t* , void* );
*/
#endif

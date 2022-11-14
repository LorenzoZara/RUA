#ifndef INTERRUPT_H
#define INTERRUPT_H

#include "../inc/libraries.h"


void interrupt_handler(int, siginfo_t *, void *);
void child_death_handler(int, siginfo_t* , void* );

#endif

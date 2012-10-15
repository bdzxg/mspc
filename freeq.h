
#ifndef _FREEQ_H_
#define _FREEQ_H_

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <stdint.h>
#include <string.h>

#define FREEQ_BUFFER_SIZE 0x3FFFFFF

void set_affinity();

typedef struct freeq_s {

	unsigned long long head;
	int p1[1000];
	unsigned long long tail;
	int p2[1000];
	int*  buffer[FREEQ_BUFFER_SIZE + 1];
} freeq_t;

freeq_t* create_free_q();
void* freeq_pop(freeq_t*);
void freeq_push(freeq_t *,void* );

#endif

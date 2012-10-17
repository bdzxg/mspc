
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <stdint.h>
#include <string.h>

#include "log.h"

#define FREEQ_BUFFER_SIZE 0x3FFFFFF

void set_affinity();

typedef struct freeq_s {

	unsigned long long head;
	int p1[1000];
	unsigned long long tail;
	int p2[1000];
	int*  buffer[FREEQ_BUFFER_SIZE + 1];
} freeq_t;

freeq_t* create_free_q()
{	
	//set_affinity();
	freeq_t *q = calloc(1, sizeof(*q));
	printf("%lu\n", sizeof(*q));
	if(!q) {
		return NULL;
	}

	q->head = 0;
	q->tail = 0;

	return q;
}

void* freeq_pop(freeq_t *q)
{
	if(q->tail <= q->head) {
		return NULL;
	}

	unsigned long long tmp = 2;
	tmp = __sync_fetch_and_add(&q->head, tmp);
	void *r;

	int i;
	while(1) {
		if( (intptr_t)q->buffer[(tmp & FREEQ_BUFFER_SIZE) + 1] == 1) {
			r = (void*)q->buffer[(tmp & FREEQ_BUFFER_SIZE)];
			q->buffer[(tmp & FREEQ_BUFFER_SIZE)] = 0;
			return r;
		}
		else {
			sched_yield();
		}
	
	}
	printf("freeq_pop return NULL,%p,head %d, tail %d \n",q, q->head, q->tail);
	return NULL;
}

void freeq_push(freeq_t *q, void* data)
{
	unsigned long long tmp = 2;
	tmp = __sync_fetch_and_add(&q->tail, tmp);
	printf("tmp is %d\n", tmp);

	do {
		if( q->buffer[(tmp & FREEQ_BUFFER_SIZE) + 1] == 0) {
			q->buffer[tmp & FREEQ_BUFFER_SIZE] = (int*)data ;
			q->buffer[(tmp & FREEQ_BUFFER_SIZE) + 1] = (int*)1;
			printf("freeq_pop pushed succ ,%p,head %d, tail %d \n",q, q->head, q->tail);
			return;
		}
		else {
			sched_yield();
		}
	} while (1);
}


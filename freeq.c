
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
	unsigned long long tmp = 2;
	tmp = __sync_add_and_fetch(&q->head, tmp);
	void *r;

	do {
		if( (intptr_t)q->buffer[(tmp & FREEQ_BUFFER_SIZE) + 1] == 1) {
			r = (void*)q->buffer[(tmp & FREEQ_BUFFER_SIZE)];
			q->buffer[(tmp & FREEQ_BUFFER_SIZE)] = 0;
			return r;
		}
		else {
			sched_yield();
		}
	} while (1);
}

void freeq_push(freeq_t *q, void* data)
{
	unsigned long long tmp = 2;
	tmp = __sync_add_and_fetch(&q->tail, tmp);

	do {
		if( q->buffer[(tmp & FREEQ_BUFFER_SIZE) + 1] == 0) {
			q->buffer[tmp & FREEQ_BUFFER_SIZE] = (int*)data ;
			q->buffer[(tmp & FREEQ_BUFFER_SIZE) + 1] = (int*)1;
			return;
		}
		else {
			sched_yield();
		}
	} while (1);
}

static int __test_count = 2000000000;
static char __test_content[15] = "hello world";

static void* test_push(void* args)
{
	freeq_t *fq = args;
	int i;
	for (i = 0; i < __test_count; i++) {
		freeq_push(fq, __test_content);
	}
	return NULL;
}

static void* test_pop(void* args)
{
	freeq_t *fq = args;
	int i;
	for (i = 0; i < __test_count; i++) {
		freeq_pop(fq);
	}
	return NULL;
}

freeq_t *q;

int main() 
{
	pthread_t* th_pop = malloc(sizeof(*th_pop));
	pthread_t* th_push = malloc(sizeof(*th_push));

	q = create_free_q();
	if(!q) {
		perror("create freeq error");
		return -1;
	}
	

	char* s = malloc(256);
	strcpy(s, "hello");
	printf("s is %s\n",s);
	free(s);
	printf("s is %s\n",s);


	printf("test begins\n");

	pthread_create(th_pop, NULL, test_pop, q);
	pthread_create(th_push, NULL, test_push, q);

	pthread_join(*th_push,NULL);
	pthread_join(*th_pop, NULL);

	printf("end");
	char c[1000];
	scanf("%s\n", c);

	return 0;
}

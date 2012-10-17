
#include "freeq.h"
#include <pthread.h>
#include <stdio.h>


static int __test_count = 10;
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
	printf("single thread test\n");

	q = create_free_q();
	if(!q) {
		perror("create freeq error");
		return -1;
	}
	
	char *c = malloc(32);
	sprintf(c, "%s", "hei");

	
	freeq_push(q, c);

	char *c1 = freeq_pop(q);


	if(c1 != NULL) {
		printf("pop content %s\n", c1);
	}
	else {
		printf("pop NULL\n");
	}


	/*  

	pthread_t* th_pop = malloc(sizeof(*th_pop));
	pthread_t* th_push = malloc(sizeof(*th_push));

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
*/
}

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "ev.h"

int log_level=0;
FILE* log_file;

void ev_callback(ev_t* ev, ev_time_item_t* ti) 
{
	printf("callback \n");
	printf("callback time %lld \n", (long long) ti->time);
}

int main()
{
	log_file = stdout;
        ev_t *ev = ev_create();
	if(!ev) {
		printf("create ev_t error \n");
		return -1;
	}

	time_t t = time(NULL);
	printf("time %d add \n", t);
	t += 5;

	ev_time_item_t* ti  = ev_time_item_new(ev, NULL, ev_callback, t);
	int r = ev_time_item_ctl(ev, EV_CTL_ADD, ti);
	if(r < 0) {
		printf("cannot all ti \n");
	}

	ev_main(ev);
	return 0;
}

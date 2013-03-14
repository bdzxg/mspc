#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>
#include "proxy.h"

int log_level=0;
FILE* log_file;

void ev_callback(ev_t* ev, ev_time_item_t* ti) 
{
	time_t now = time(NULL);
	D("RUN ! current time is %d , expect time %lld \n", now, (long long) ti->time);
	free(ti);
}

int main()
{
	log_file = stdout;
        ev_t *ev = ev_create();
	if(!ev) {
		D("create ev_t error \n");
		return -1;
	}

	time_t t = time(NULL);
	D("time %d add \n", t);

	ev_time_item_t* ti  = ev_time_item_new(ev, NULL, ev_callback, t+5);
	int r = ev_time_item_ctl(ev, EV_CTL_ADD, ti);
	if(r < 0) {
		D("cannot all ti \n");
	}
	
	ev_time_item_t* ti2 = ev_time_item_new(ev, ti, ev_callback, t+2);
	D("2");
	if(ev_time_item_ctl(ev, EV_CTL_ADD, ti2) < 0) {
		D("cannot add ti2 \n");
	}

	D("2");
	int i = 0;
	for(; i < 10000 ; i++) {
		ti = ev_time_item_new(ev, NULL, ev_callback, t + 100);
		if(ev_time_item_ctl(ev, EV_CTL_ADD, ti) < 0) {
			E("cannnot add ti");
		}
	}
	D("3");
	if(ev_time_item_ctl(ev, EV_CTL_DEL, ti) < 0) {
		E("cannot delete last item");
	}

	D("finish");
	
	ev_main(ev);
	return 0;
}

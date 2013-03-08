
/*only for epoll now*/

#include "proxy.h"

ev_t*
ev_create(void* data)
{
	ev_t* ev;
	int fd = epoll_create(1024/*kernel hint*/);

	if(fd > 0){
		ev = (ev_t*)calloc(1, sizeof(ev_t));
		if(!ev) {
			return NULL;
		}
		ev->fd = fd;
		ev->data = data;
		ev->next_time_id = 0;
		ev->ti = NULL;
		ev->api_data=malloc(sizeof(struct epoll_event)*EV_COUNT);
		ev->after = NULL;
		ev->root = RB_ROOT;
		return ev;
	}

	return NULL;
}

int 
ev_add_file_item(ev_t* ev,ev_file_item_t* item)
{
	struct epoll_event epev;

	epev.events = item->events;
	//epev.data.fd = item->fd;
	epev.data.ptr = item;
  
	int i = epoll_ctl(ev->fd,EV_CTL_ADD,item->fd, &epev);
	return i;
}

int 
ev_del_file_item(ev_t *ev, ev_file_item_t* item)
{
	struct epoll_event epev;
	epev.events = item->events;
	epev.data.ptr = item;
	return epoll_ctl(ev->fd,EV_CTL_DEL,item->fd, &epev);
}

int 
ev_time_item_ctl(ev_t* ev, int op, ev_time_item_t* item)
{
	int idx;
	time_t t = time(NULL);
	time_t p = item->time - t;

	if(op == EV_CTL_ADD) {
		if(p > 3600 || p < 0) {
			//only support 1 hour
			return -1;
		}
		idx = (p + ev->timer_current_task) % EV_TIMER_SIZE;
		ev_time_item_t* tmp = ev->timer_task_list[idx];
		item->_next = tmp;
		ev->timer_task_list[idx] = item;
		//TODO: add to the tree
		return 1;
	}
	else if (op == EV_CTL_DEL) {
		//TODO: delete item from the tree
		item->deleted = 1;
	}
	return 0;
}

void
ev_main(ev_t* ev)
{
	D("ev_main started");
	while(ev->stop <= 0) {
		//try to process the timer first 
		int idx = ev->timer_current_task % EV_TIMER_SIZE;
		ev_time_item_t *ti = ev->timer_task_list[idx];
		time_t now = time(NULL);
		while(ti) {
			if(ti->deleted == 1) {
				ev_time_item_t* tmp = ti;
				ev->timer_task_list[idx] = ti->_next;
				ti = ti->_next;
				free(tmp);
			}
			else if (ti->time > now) {
				break;
			}
			else {
				ti->func(ev, ti);
				free(ti);
			}
		}
		if(!ti) {
			ev->timer_current_task++;
		}
		

		int i,j;
		struct epoll_event* e = ev->api_data;
		j = epoll_wait(ev->fd,e,EV_COUNT,100);
		for(i=0; i<j ;i++){
			ev_file_item_t* fi = (ev_file_item_t*)e[i].data.ptr; 
			int evts = e[i].events;
			
			D("events %d", evts);
			if(evts & EPOLLIN) {
				if(fi->rfunc){
					D("RFUNC");
					if(fi->rfunc(ev,fi) < 0)
						continue;
				}
			}
			
			if(evts & EPOLLOUT || evts & EPOLLHUP || evts & EPOLLERR) {
				if(fi->wfunc) {
					D("WFUNC");
					fi->wfunc(ev,fi);
				}
			}
		}

		//Handle the after event Handle
		if(ev->after != NULL) {
			ev->after(ev);
		}
		
	} /*while*/
	D("worker EV_MAIN stopped");
}

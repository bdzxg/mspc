
/*only for epoll now*/

#include "proxy.h"

ev_t* ev_create2(void* data, size_t size)
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
		if(!ev->api_data) {
			free(ev);
			return NULL;
		}
		ev->events_size = size;
		ev->events = calloc(size, sizeof(ev_file_item_t));
		if(!ev->events) {
			free(ev->api_data);
			free(ev);
			return NULL;
		}
		ev->after = NULL;
		ev->root = RB_ROOT;
		return ev;
	}

	return NULL;

}

ev_t* ev_create(void* data)
{
	return ev_create2(data, 1024);
}
	
int ev_add_file_item(ev_t* ev, int fd, int mask, void* d,
		     ev_file_func* rf, ev_file_func* wf)
{
	if(fd >= ev->events_size) {
		W("fd %d is large than size : %zu", 
		  fd, ev->events_size);
		return -1;
	}
	ev_file_item_t *item = ev->events + fd;
	if(item->mask > 0) {		
		return -1;
	}

	struct epoll_event epev;
	epev.events = mask;
	epev.data.fd = fd;
	
	if(epoll_ctl(ev->fd, EV_CTL_ADD, fd, &epev) == 0) {
		item->mask = mask;
		item->data = d;
		item->wfunc = wf;
		item->rfunc = rf;
		item->fd = fd;
		return 0;
	}
	return -1;
}

int ev_del_file_item(ev_t *ev, int fd)
{
	if(fd >= ev->events_size) return -1;
	ev_file_item_t* fi = ev->events + fd;
	if(fi->mask == 0) return 0;
	
	fi->mask = 0;
	int op = EV_CTL_DEL;

	struct epoll_event epev;
	epev.events = fi->events;
	epev.data.fd = fd;
	return epoll_ctl(ev->fd, op ,fd, &epev);
}

static int _ev_insert_timer_tree(ev_t *ev, ev_time_item_t* ti)
{
	struct rb_root *root = &ev->root;
	struct rb_node **new = &(root->rb_node), *parent = NULL;

	while(*new) {
		ev_time_item_t *data = rb_entry(*new, struct ev_time_item_s, rbnode);
		int result = data->id - ti->id;
		parent = *new;
		if(result < 0)
			new = &((*new)->rb_left);
		else if(result > 0)
			new = &((*new)->rb_right);
		else 
			return -1;
	}

	rb_link_node(&ti->rbnode,parent,new);
	rb_insert_color(&ti->rbnode,root);
	return 1;
}

static void  _ev_delete_from_timer_tree(ev_t* ev, ev_time_item_t* ti)
{
	struct rb_root *root = &ev->root;
	struct rb_node *node = root->rb_node;

	while(node) {
		ev_time_item_t *data = rb_entry(node,struct ev_time_item_s,rbnode);
		int result = data->id - ti->id;

		if (result < 0)
			node = node->rb_left;
		else if (result > 0)
			node = node->rb_right;
		else {
			rb_erase(node, &ev->root);
			return;
		}
	}
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
		if(_ev_insert_timer_tree(ev, item) < 0) {
			return -1;
		}
		idx = (p + ev->timer_current_task) % EV_TIMER_SIZE;
		ev_time_item_t* tmp = ev->timer_task_list[idx];
		item->_next = tmp;
		ev->timer_task_list[idx] = item;
	}
	else if (op == EV_CTL_DEL) {
		_ev_delete_from_timer_tree(ev, item);
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
			ev_time_item_t* tmp = ti;
			if(ti->deleted == 1) {
				D("got delete item %llu", ti->id);
				ev->timer_task_list[idx] = ti->_next;
				ti = ti->_next;
				free(tmp);
			}
			else if (ti->time > now) {
				D("the timer should be called later");
				break;
			}
			else {
				D("got timer #%llu to run", ti->id);
				ti->func(ev, ti);
				ev->timer_task_list[idx] = ti->_next;
				ti = ti->_next;
				free(tmp);
			}
		}
		if(!ti) {
			ev->timer_current_task++;
		}
		
		//process file events
		int i,j;
		struct epoll_event* e = ev->api_data;
		j = epoll_wait(ev->fd,e,EV_COUNT,100);
		for(i=0; i<j ;i++){
			int fd = e[i].data.fd;
			ev_file_item_t* fi = ev->events + fd;
			int evts = e[i].events;
			
			D("events %d", evts);
			if(evts & EV_READABLE & fi->mask) {
				if(fi->rfunc){
					D("RFUNC");
					fi->rfunc(ev,fi);
				}
			}
			
			if(evts & EV_WRITABLE & fi->mask) {
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

#ifndef _EV_H_
#define _EV_H_

#define EV_READABLE EPOLLIN
#define EV_WRITABLE EPOLLOUT
#define EV_ALL 3

#define EV_TIME 1
#define EV_FILE 2

/*same to epoll now*/
#define EV_CTL_ADD 1
#define EV_CTL_DEL 2
#define EV_CTL_MOD 3

#define EV_COUNT 10000

#include <stdint.h>
#include "rbtree.h"

struct ev_s;
struct ev_time_item_s;
struct ev_file_item_s;

typedef void ev_time_func(struct ev_s* ev, struct ev_time_item_s* ti);
typedef int ev_file_func(struct ev_s* ev,struct ev_file_item_s* fi);
typedef void ev_after_event_handle(struct ev_s *ev);

typedef struct ev_time_item_s{
	unsigned long long id;
	time_t time;
	void* data;
	int deleted;
	struct ev_time_item_s* _next;
	ev_time_func* func;
	struct rb_node rbnode;
}ev_time_item_t;

typedef struct ev_file_item_s{
	int fd;
	int mask;
	int events;
	void* data;
	ev_file_func* wfunc;
	ev_file_func* rfunc;
	int valid;
}ev_file_item_t;

typedef struct ev_fired_s{
	int fd;
	int mask;
}ev_fired_t;


#define EV_TIMER_SIZE 3600

typedef struct ev_s{
	int fd;
	void *data;
	uint64_t next_time_id;
	void *timer_task_list[EV_TIMER_SIZE];
	uint32_t timer_current_task;
	struct rb_root root;
	ev_time_item_t* ti;
	void* api_data;
	int stop;
	ev_after_event_handle *after;
}ev_t;


#define ev_get_current_ms(__m)			\
	({					\
		struct timeval __tv;		\
		gettimeofday(&__tv,NULL);	\
		*__m = __tv.tv_usec/1000;	\
	})					\

static inline ev_file_item_t*
ev_file_item_new(int fd, void* data, ev_file_func* rf,
		 ev_file_func* wf, int evts)
{
	ev_file_item_t *fi = malloc(sizeof(*fi));
	if(!fi) {
		return NULL;
	}
	
	fi->fd = fd;
	fi->data = data;
	fi->wfunc = wf;
	fi->rfunc = rf;
	fi->valid = 1;
	fi->events = evts;

	return fi;
}

static inline ev_time_item_t*
ev_time_item_new(ev_t* ev, void* d, ev_time_func* f, time_t t)
{
	ev_time_item_t* ti = calloc(1, sizeof(*ti));
	if(!ti) {
		return NULL;
	}
	ti->id = ++ ev->next_time_id;
	ti->time = t;
	ti->data = d;
	ti->func = f;
	ti->deleted = 0;
	return ti;
}

ev_t* ev_create();
int ev_time_item_ctl(ev_t* ev,int op,ev_time_item_t* item);
int ev_add_file_item(ev_t*,ev_file_item_t*);
int ev_del_file_item(ev_t*,ev_file_item_t*);
void ev_main(ev_t* ev);

#endif

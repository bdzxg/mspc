#ifndef __UPSTEAM_H
#define __UPSTEAM_H

#include <string.h>
#include "proxy.h"

typedef int us_be_init_func();
typedef int us_be_start_func();
typedef int us_send_func(rec_msg_t*);
typedef void us_send_err_func(rec_msg_t *);

typedef struct upstream_s {
	char name[32];
	us_be_start_func *start;
	us_be_init_func *init;
	us_send_func *send;
	us_send_err_func *send_err;

	char *uri;
	struct rb_node rbnode;
	list_head_t conn_list;
	list_head_t pending_list;
}upstream_t;

typedef struct upstream_map_s {
	struct rb_root root;
	size_t count;
	list_head_t conn_list;
	size_t conn_count;
} upstream_map_t;

int us_add_upstream(upstream_map_t*, upstream_t*);
upstream_t* us_get_upstream(upstream_map_t *, char*);
void release_upstream(upstream_t* us);


#endif


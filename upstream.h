
#ifndef __UPSTEAM_H
#define __UPSTEAM_H

#include <string.h>
#include "proxy.h"

#define UP_CONN_COUNT 3

typedef struct upstream_s {
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


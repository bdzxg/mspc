
#ifndef __UPSTEAM_H
#define __UPSTEAM_H

#include <string.h>
#include "proxy.h"
#include "include/rpc_client.h"

typedef struct upstream_s {
	char *uri;
	rpc_proxy_t *proxy;
	struct rb_node rbnode;
}upstream_t;

typedef struct upstream_map_s {
	struct rb_root root;
	size_t count;
} upstream_map_t;

int us_add_upstream(upstream_map_t*, upstream_t*);
upstream_t* us_get_upstream(upstream_map_t *, char*);
void release_upstream(upstream_t* us);







#endif


#ifndef _MAP_H
#define _MAP_H

#include "proxy.h"

#define map_walk(root,rn)				\
	for((rn)=rb_first((root));rn;(rn)=rb_next((rn)))

pxy_agent_t* map_search(struct rb_root *,char *);
pxy_agent_t* map_remove(struct rb_root *, char *);
int map_insert(struct rb_root *, pxy_agent_t *);

#endif

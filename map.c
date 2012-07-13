#include <string.h>
#include "proxy.h"

int map_insert(struct rb_root *root, pxy_agent_t *q)
{
	struct rb_node **new = &(root->rb_node), *parent = NULL;

	while(*new) {
		pxy_agent_t *data = rb_entry(*new,struct pxy_agent_s,rbnode);
		int result = strcmp(data->epid,q->epid);
		parent = *new;
		if(result < 0)
			new = &((*new)->rb_left);
		else if(result > 0)
			new = &((*new)->rb_right);
		else 
			return -1;
	}

	rb_link_node(&q->rbnode,parent,new);
	rb_insert_color(&q->rbnode,root);
	return 1;
}

pxy_agent_t* map_search(struct rb_root *root,char *name)
{
	struct rb_node *node = root->rb_node;

	while(node) {
		pxy_agent_t *q = rb_entry(node,struct pxy_agent_s,rbnode);
		int result = strcmp(q->epid,name);

		if (result < 0)
			node = node->rb_left;
		else if (result > 0)
			node = node->rb_right;
		else 
			return q;
	}

	return NULL;
}

pxy_agent_t* map_remove(struct rb_root *root, char *name)
{
	pxy_agent_t *data = map_search(root,name);

	if(data) {
		rb_erase(&data->rbnode,root);
	}
	return data;
}




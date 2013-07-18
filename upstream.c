#include "upstream.h"

int us_add_upstream(upstream_map_t *map, upstream_t *us)
{
	struct rb_root *root = &map->root;
	struct rb_node **new = &(root->rb_node), *parent = NULL;

	while (*new) {
		upstream_t *data = rb_entry(*new, struct upstream_s, rbnode);
		int result = strcmp(data->uri,us->uri);
		parent = *new;
		if (result < 0)
			new = &((*new)->rb_left);
		else if (result > 0)
			new = &((*new)->rb_right);
		else 
			return -1;
	}

	rb_link_node(&us->rbnode,parent,new);
	rb_insert_color(&us->rbnode,root);
	return 1;
}

upstream_t* us_get_upstream(upstream_map_t *map, char *uri)
{
	struct rb_root *root = &map->root;
	struct rb_node *node = root->rb_node;

	while (node) {
		upstream_t *q = rb_entry(node, struct upstream_s, rbnode);
		int result = strcmp(q->uri, uri);

		if (result < 0)
			node = node->rb_left;
		else if (result > 0)
			node = node->rb_right;
		else 
			return q;
	}

	return NULL;
}

//TODO:
/*
static pxy_agent_t* us_remove_upstream(struct rb_root *root, char *name)
{
	pxy_agent_t *data = map_search(root,name);

	if (data) {
		rb_erase(&data->rbnode,root);
	}
	return data;
}*/
void release_upstream(upstream_t* us)
{
	free(us->uri);
	//TODO
}
  

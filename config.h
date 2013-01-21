#ifndef _CONFIG_H
#define _CONFIG_H

#define CONFIG_MAX 256

typedef struct config_item {
	char name[CONFIG_MAX];
	char value[CONFIG_MAX];
	struct config_item *next;
} config_item;

int load_config(FILE *,config_item*);
#endif

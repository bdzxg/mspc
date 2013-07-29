#include <stdio.h>
#include <string.h>
#include "config.h"

#define CONF_BUF_LEN  2 * CONFIG_MAX + 1
static char conf_buf[CONF_BUF_LEN] = { 0 };

int load_config(FILE *file, config_item *item)
{
	int len;
	char *c;

	if (fgets(conf_buf, CONF_BUF_LEN, file) == NULL) {
		 return 0; /* EOF */
	}
	
	if (conf_buf[0] == '#') {
		return -1;
	}

	c = index(conf_buf, ' ');
	if (c == NULL) {
		return -1;
	}

	len = c - conf_buf;
	if (len > CONFIG_MAX) {
		return -1;
	}
	strncpy(item->name, conf_buf , len);

	if (strlen(c) > CONFIG_MAX + 1) {
		return -1;
	}
	//skip the middle space and the last '\n'
	if (c[strlen(c)-1] == '\n')
		strncpy(item->value, c + 1, strlen(c) - 2); 
	else 
		strncpy(item->value, c + 1, strlen(c) - 1); 
	return 1;
}

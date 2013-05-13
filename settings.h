#ifndef _SETTING_H
#define _SETTING_H

#include "log.h"
#include "config.h"

typedef struct pxy_settings{
	int log_level;
	char log_file[CONFIG_MAX];
	int client_port;
	int backend_port;
	char ip[CONFIG_MAX];
	int route_log_level;
	char route_log_file[CONFIG_MAX];
        int route_server_port;
	char zk_url[CONFIG_MAX];
} pxy_settings;

#endif

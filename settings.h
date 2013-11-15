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
        int is_flush_log;
        int check_client_alive_time;
        int transaction_timeout;
	char dblog_host[CONFIG_MAX];
	char dblog_user[CONFIG_MAX];
	char dblog_passwd[CONFIG_MAX];
	char dblog_db[CONFIG_MAX];
	int dblog_port;
} pxy_settings;

#endif

#ifndef _SETTING_H
#define _SETTING_H

#include "log.h"
#include "config.h"

typedef struct pxy_settings{
	int log_level;
	char log_file[CONFIG_MAX];
	int client_port;
	int backend_port;
} pxy_settings;

#endif

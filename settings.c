#include "settings.h"

pxy_settings setting = {
        LOG_LEVEL_DEBUG,
	"mspc.txt",
        9014,
        9015,
	"0.0.0.0",
	LOG_LEVEL_DEBUG,
	"route_log.txt",
        10000,
	"192.168.110.125:8998",
        0,  //flushlog
        300,  //close_idle client_time
        30,  //transaction timeout value
        "127.0.0.1",
        "admin",
        "admin",
        "LogDB", 
        3306
};

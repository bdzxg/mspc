/*
 * rpc_log.h
 *
 *  Created on: 2012-2-22
 *      Author: yanghu
 */

#ifndef _RPC_LOG_H_INCLUDED_ 
#define _RPC_LOG_H_INCLUDED_

#include <sys/time.h>
#include <syslog.h>
#include <libgen.h>

#define RPC_LOG_LEVEL_NONE 0
#define RPC_LOG_LEVEL_DEBUG 1
#define RPC_LOG_LEVEL_INFO 2
#define RPC_LOG_LEVEL_WARN 3
#define RPC_LOG_LEVEL_ERROR 4

#ifndef RPC_LOG_LEVEL
#define RPC_LOG_LEVEL RPC_LOG_LEVEL_ERROR
#endif

#ifdef RPC_LOG_TIME
#define RPC_LOG_TIME_FORMAT "%llu:%llu | "
#define RPC_LOG_TIME_VAR struct timeval _tv;  gettimeofday(&_tv, NULL);
#define RPC_LOG_TIME_VAL (unsigned long long)_tv.tv_sec, (unsigned long long)_tv.tv_usec,
#else
#define RPC_LOG_TIME_FORMAT
#define RPC_LOG_TIME_VAR
#define RPC_LOG_TIME_VAL
#endif

#ifndef RPC_LOG_PTR
#define RPC_LOG(l, fmt, args...) do {                                           \
    RPC_LOG_TIME_VAR                                                           \
    syslog((l), RPC_LOG_TIME_FORMAT "%s | %s (line: %d) : " fmt ,               \
            RPC_LOG_TIME_VAL                                                    \
            basename(__FILE__), __func__, __LINE__, ## args);                   \
    } while(0)
#else
#define RPC_LOG(l, fmt, args...) do {                                           \
    RPC_LOG_TIME_VAR                                                            \
    fprintf(stderr, RPC_LOG_TIME_FORMAT "%s | %s (line: %d) : " fmt "\n",           \
            RPC_LOG_TIME_VAL                                                    \
            basename(__FILE__), __func__, __LINE__, ## args);                   \
} while(0)
#endif

#if (RPC_LOG_LEVEL == RPC_LOG_LEVEL_DEBUG)
#define rpc_log_debug(f, args...) RPC_LOG(LOG_DEBUG, f, ## args)
#define rpc_log_info(f, args...) RPC_LOG(LOG_INFO, f, ## args)
#define rpc_log_warn(f, args...) RPC_LOG(LOG_WARNING, f, ## args)
#define rpc_log_error(f, args...) RPC_LOG(LOG_ERR, f, ## args)
#elif (RPC_LOG_LEVEL == RPC_LOG_LEVEL_INFO)
#define rpc_log_debug(f, args...) 
#define rpc_log_info(f, args...) RPC_LOG(LOG_INFO, f, ## args)
#define rpc_log_warn(f, args...) RPC_LOG(LOG_WARNING, f, ## args)
#define rpc_log_error(f, args...) RPC_LOG(LOG_ERR, f, ## args)
#elif (RPC_LOG_LEVEL == RPC_LOG_LEVEL_WARN)
#define rpc_log_debug(f, args...) 
#define rpc_log_info(f, args...) 
#define rpc_log_warn(f, args...) RPC_LOG(LOG_WARNING, f, ## args)
#define rpc_log_error(f, args...) RPC_LOG(LOG_ERR, f, ## args)
#elif (RPC_LOG_LEVEL == RPC_LOG_LEVEL_ERROR)
#define rpc_log_debug(f, args...) 
#define rpc_log_info(f, args...) 
#define rpc_log_warn(f, args...) 
#define rpc_log_error(f, args...) RPC_LOG(LOG_ERR, f, ## args)
#elif (RPC_LOG_LEVEL == RPC_LOG_LEVEL_NONE)
#define rpc_log_debug(f, args...) 
#define rpc_log_info(f, args...) 
#define rpc_log_warn(f, args...) 
#define rpc_log_error(f, args...) 
#endif

#endif


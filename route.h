/*
 * =====================================================================================
 *
 *       Filename:  route.h
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  03/22/2012 09:55:11 AM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  YOUR NAME (), 
 *        Company:  
 *
 * =====================================================================================
 */

#ifndef __ROUTE_H_
#define __ROUTE_H_

#include "fae_application_args.pb-c.h"
#include "fae_running_worker_args.pb-c.h"
#include "include/list.h"
#include <time.h>
#include "include/zookeeper.h"
#include "gray/gray.h"

static  char *PROTO_RPC = "rpc";
static  char *PROTO_HTTP = "http";
static  zhandle_t *zh;

struct hashtable *ht_applications;
struct hashtable *ht_runningworkers;
struct hashtable *ht_inject_args;

pthread_rwlock_t app_mutex;
pthread_rwlock_t wk_mutex;
pthread_rwlock_t inject_args_mutex;

typedef void* route_ctx_clone(void*);
typedef void* route_ctx_getfield(void*, char*);

typedef struct route_ctx_func {
	route_ctx_clone *clone;
	route_ctx_getfield *get_field;
}route_ctx_func;

typedef struct mcp_appbean_params_s
{
    short protocol;
    unsigned int cmd;
} mcp_appbean_params_t;

typedef struct route_app_s
{ 
    char *appworkerid;
    char *grayfactors;
    char *category;
    char *name;
    char *endpoint; //only used for injection
}route_app_t;

typedef struct mcp_appbean_value_s
{
    route_app_t *apps;
    list_head_t list;
} mcp_appbean_value_t;

typedef struct mcp_runningworker_param_s
{
    char* appworkerid;
}mcp_runningworker_param_t; 

typedef struct route_worker_s
{
    char *appworkerid;
    char *servername;
    char *rpc_url;
    uint64_t lasttime;
}route_worker_t;

typedef struct mcp_runningworker_value_s
 {
    route_worker_t *workers;
    int wk_count;
    list_head_t list;
}mcp_runningworker_value_t;


int get_app_url(const unsigned int cmd, const short protocol, const char* uid, const char* sid, 
        const char* client_type, const char* client_version, char* url);

int get_app_category_minus_name(const unsigned int cmd, const short protocol, char *buffer);

int get_app_count();

int route_init(char *zookeeper_url);

void route_close();

char *get_value(gray_user_context_t *ctx, const char *field_name);

#endif


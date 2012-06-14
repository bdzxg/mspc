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

static  char *PROTO_RPC = "rpc";
static  char *PROTO_HTTP = "http";
static  zhandle_t *zh;

typedef struct mcp_appbean_params_s
{
    short protocol;
    unsigned short cmd;
} mcp_appbean_params_t;

typedef struct mcp_appbean_value_s
{
    char *appworkerid;
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

/*
 * cmd: CMD CODE, 
 * protocol: MCP/3.0 = 1
 * sid:
 * client_type:
 * client_version:
 * url: url for app rpc call 
 */
int get_app_url(const unsigned short cmd, const short protocol, const char* sid,
        const char* client_type, const char* client_version, char** url);

int route_init(char *zookeeper_url);

void route_close();

void test();

#endif


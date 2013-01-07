/*
 * =============================================================================
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
 * =============================================================================
 */

#ifndef __ROUTE_H_
#define __ROUTE_H_

typedef struct route_user_context_s route_user_context_t;


typedef void* route_ctx_clone(void*);
typedef void* route_ctx_getfield(void*, char*);

typedef struct route_ctx_func{
        route_ctx_clone *clone;
        route_ctx_getfield *get_field;
}route_ctx_func;

struct route_user_context_s {
        char *uid;
        char *sid;
	char *mobile_no;
	char *user_region;
	char *user_carrier;
	char *client_type;
	char *client_region;
	char *client_version;
	char * (*get_value)(route_user_context_t *ctx, const char *field_name);
};

int route_get_app_url(const unsigned int cmd, const short protocol, 
                const char *uid, const char* sid, const char* client_type, 
                const char* client_version, char* url);

int route_get_app_category_minus_name(const unsigned int cmd, 
                                      const short protocol,char *buffer);

int route_get_app_count();

int route_init(char *zookeeper_url);

void route_close();

char *route_get_ctx(route_user_context_t *ctx, const char *field_name);

#endif


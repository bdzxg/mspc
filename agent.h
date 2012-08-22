#ifndef _AGENT_H_
#define _AGENT_H_
#include <stdio.h>
#include "proxy.h"
#include "include/rpc_args.h"

typedef struct pxy_agent_s{
    struct buffer_s *buffer;
    int fd;
    int user_id;
	int bn_seq;
	char* epid;
    size_t buf_offset;/*all data len in buf*/
    size_t buf_parsed;
    size_t buf_sent;
    size_t buf_list_n; /* struct buffer_s count */
	size_t buf_len;
	struct rb_node rbnode;
	char* user_ctx;
	int user_ctx_len;
	UserContext user_context;  
	int msp_unreg;
	int logic_pool_id;
	char* epidr2;
}pxy_agent_t;

typedef struct message_s {
    uint32_t len;
    uint32_t cmd;
    char *body;
}message_t;

typedef struct rec_message_s{
    int len;
	char version;
    int userid;
	int cmd;
    int seq;
	int off;
	int format;	
	int compress;
	int client_type;
	int client_version;
	int logic_pool_id;
	char *option;
	int option_len;
	char *body;
	int body_len;
	char *user_context;
	int user_context_len;
	char *epid;
}rec_msg_t;

pxy_agent_t* pxy_agent_new(mp_pool_t *,int,int);
void pxy_agent_close(pxy_agent_t *);
int pxy_agent_data_received(pxy_agent_t *);
int pxy_agent_upstream(int ,pxy_agent_t *);
int pxy_agent_echo_test(pxy_agent_t *);
int pxy_agent_buffer_recycle(pxy_agent_t *);
buffer_t* agent_get_buf_for_read(pxy_agent_t*);
int process_received_msg(size_t buf_size, uint8_t* buf_ptr, rec_msg_t* msg);
void agent_recv_client(ev_t *,ev_file_item_t*);
void free_string_ptr(char* str);
void agent_send_client(rec_msg_t* rec_msg, pxy_agent_t *agent);
void send_response_client(rec_msg_t* req,  pxy_agent_t* a, int code); 

#define pxy_agent_for_each(agent,alist)			\
    list_for_each_entry((agent),list,&(alist)->list)	

#define pxy_agent_append(agent,alist)		\
    list_append(&(agent)->list,&(alist)->list)		

#define pxy_agent_remove(agent)			\
    list_remove(&(agent)->list)

#endif

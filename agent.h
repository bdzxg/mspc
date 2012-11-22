#ifndef _AGENT_H_
#define _AGENT_H_
#include "proxy.h"

typedef struct pxy_agent_s{
	int fd;
	int user_id;
	int bn_seq;
	char* epid;
	mrpc_buf_t *send_buf;
	mrpc_buf_t *recv_buf;
	struct rb_node rbnode;
	char* user_ctx;
	int user_ctx_len;
	int logic_pool_id;
	char* epidr2;
	size_t isreg;
	int clienttype;
	long upflow;
	long downflow;
	char* logintime;
	char* logouttime;
	ev_file_item_t *ev;
}pxy_agent_t;

typedef struct reg3_s{
	int user_id;
	char* epid;
	int logic_pool_id;
	char *user_context;
	int user_context_len;
	time_t remove_time;
	struct rb_node rbnode;
}reg3_t;

typedef struct message_s {
	uint32_t len;
	uint32_t cmd;
	char *body;
}message_t;

typedef struct rpc_async_req_s{
	pxy_agent_t* a;
	rec_msg_t* req;
	void* rpc_bf;
	int msp_unreg;
}rpc_async_req_t;


pxy_agent_t* pxy_agent_new(mp_pool_t *,int,int);
void pxy_agent_close(pxy_agent_t *);
int pxy_agent_data_received(pxy_agent_t *);
int pxy_agent_upstream(int ,pxy_agent_t *);
int pxy_agent_echo_test(pxy_agent_t *);
int pxy_agent_buffer_recycle(pxy_agent_t *);
int process_received_msg(size_t buf_size, uint8_t* buf_ptr, rec_msg_t* msg);
int agent_recv_client(ev_t *,ev_file_item_t*);
void free_string_ptr(char* str);
void agent_send_client(rec_msg_t* rec_msg, pxy_agent_t *agent);
void send_response_client(rec_msg_t* req,  pxy_agent_t* a, int code); 
void agent_mrpc_handler(mcp_appbean_proto *proto);
int worker_insert_agent(pxy_agent_t *agent);
void worker_remove_agent(pxy_agent_t *agent);
void worker_insert_reg3(reg3_t* r3);
reg3_t* worker_remove_reg3(char* key);
void release_reg3(reg3_t* r3);
int store_connection_context(pxy_agent_t *a);
void worker_recycle_reg3();
void send_to_client(pxy_agent_t*, char*, size_t);

#define pxy_agent_for_each(agent,alist)				\
	list_for_each_entry((agent),list,&(alist)->list)	

#define pxy_agent_append(agent,alist)			\
	list_append(&(agent)->list,&(alist)->list)		

#define pxy_agent_remove(agent)			\
	list_remove(&(agent)->list)

#endif

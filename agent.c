#include "proxy.h"
#include "agent.h"
#include "include/zookeeper.h"
#include <pthread.h>
#include "route.h"
#include "ClientHelper.c"

extern pxy_worker_t *worker;
extern upstream_map_t *upstream_root;

size_t packet_len;
#define MSP_BUF_LEN 1024

static char OVERTIME[3] = {8, 248, 3};// 504
static char BADGATEWAY[3] = {8, 246, 3};// 502
static char SERVERERROR[3] = {8, 244, 3};// 500
static char REQERROR[3] = {8, 144, 3};// 400
static char NOTEXIST[3] = {8, 155, 3};// 411

static int agent_to_beans(pxy_agent_t *a, rec_msg_t* msg, int msp_unreg);
int agent_send_netstat(pxy_agent_t *agent);
int agent_send_offstate(a);
int worker_insert_agent(pxy_agent_t *agent)
{
	int ret = 0;
	ret = map_insert(&worker->root,agent);
	return ret;
}

void worker_remove_agent(pxy_agent_t *agent)
{
	if(agent->epid != NULL) {
		map_remove(&worker->root, agent->epid);
	}
}

void worker_insert_reg3(reg3_t* r3)
{
	map_insert_reg3(&worker->r3_root, r3);	
}

/*
void msp_send_unreg(reg3_t* a)
{
	if(a->user_context!= NULL) {
		rec_msg_t msg;
		msg.cmd = 103;
		msg.userid = a->user_id;
		msg.logic_pool_id = a->logic_pool_id;
		msg.seq = 100001;
		msg.format = 0;
		msg.user_context_len = a->user_context_len;
		msg.user_context = a->user_context;
		msg.body_len = 0;
		msg.epid = a->epid;
		msg.compress = 0;
		agent_to_beans(NULL, &msg, 1);
	}
}
*/


//ok 1, not finish 0, error -1
int parse_client_data(pxy_agent_t *agent, rec_msg_t* msg)
{
	mrpc_buf_t *b = agent->recv_buf;
#define __B (b->buf + b->offset)
#define __B1 (b->buf + b->offset++)
	size_t data_len = b->size - b->offset;
	
	if(data_len < 2) {
		D("length %lu is not enough", data_len);
		return 0;
	}

	msg->len = (int)le16toh(*(uint16_t*)__B);
	D("b->offset is %zu, msg->len %d", b->offset, msg->len);
	if(data_len < msg->len) {
		D("%zu < %d, wait for more data",data_len, msg->len);
		return 0;
	}	
	b->offset += 2;
	
	msg->version = *__B1;
	msg->userid = le32toh(*(uint32_t*)__B);
	b->offset += 4;
	msg->cmd = le32toh(*(uint32_t*)__B);
	b->offset += 4;
	msg->seq = le16toh(*(uint16_t*)__B);
	b->offset += 2;
	msg->off = *__B1;
	msg->format = *__B1;
	msg->compress = *__B1;
	msg->client_type = le16toh(*(uint16_t*)__B);
	b->offset += 2;
	msg->client_version = le16toh(*(uint16_t*)__B);
	b->offset += 2;


	b->offset ++;	//oimt the empty byte
	msg->option_len = msg->off - 21; //21 is the fixed length of header
	b->offset += msg->option_len;

	msg->body_len = msg->len - msg->off;
	D("body_len %d", msg->body_len);

	if(agent->user_ctx_len == 0){
		msg->user_context_len = 0; 
		msg->user_context = NULL;
	}else{
		msg->user_context_len = agent->user_ctx_len;
		msg->user_context = agent->user_ctx;
	}

	if(msg->body_len >0){
		msg->body = malloc(msg->body_len);
		if(msg->body == NULL)
			return -1;

		memcpy(msg->body, b->buf + b->offset,msg->body_len);
	}
	else{
		msg->body = NULL;
		msg->body_len = 0; 
	}
	b->offset += msg->body_len;

	//packet_len = 0;

	if(agent->epid != NULL) {
		msg->epid = agent->epidr2; 
		/* 
		if(msg->cmd == 102)
			msg->epid = agent->epidr2; 
		else
			msg->epid = agent->epid; 
 		*/
	}
	else {
		char* t = generate_client_epid(msg->client_type, msg->client_version);
		msg->epid = agent->epid = t;
		agent->epidr2 = calloc(strlen(agent->epid)+strlen(LISTENERPORT)+2, 1);		
		sprintf("%s,%s", agent->epid, LISTENERPORT);
		agent->clienttype = msg->client_type;

		worker_insert_agent(agent);
	}
	//TODO: change this
	msg->epid = agent->epidr2; 
	D("msp->epid %s", msg->epid);
	msg->logic_pool_id = agent->logic_pool_id;
	return 1;
}


static inline void 
_msg_from_proto(mcp_appbean_proto *p, rec_msg_t *m)
{
	memset(m, 0, sizeof(*m));

	m->userid = p->user_id;
	m->seq = p->sequence;
	m->cmd = p->cmd;
	m->format = p->opt;
	m->compress = p->zip_flag;

	if(p->content.len > 0) {
		m->body_len = p->content.len;
		m->body = p->content.buffer;
	}
}

#define CMD_USER_VALID 101
#define CMD_CLOSE_CONNECTION 102
#define CMD_CONNECTION_CLOSED 103
#define CMD_NET_STAT 104
#define CMD_REFRESH_EPID 105


static int
 _refresh_agent_epid(pxy_agent_t *a, struct pbc_slice *slice)
{
	char* t = calloc(slice->len + 1, 1);
	if(!t) {
		E("cannnot malloc for new epid");
		return -1;
	}
	char *t2 = calloc(slice->len + strlen(LISTENERPORT) + 2 , 1);
	if(!t2) {
		E("cannnot malloc for new epid2");
		free(t);
		return -1;
	}

	free(a->epid);
	free(a->epidr2);

	strncpy(t, slice->buffer, slice->len);
	sprintf(t2, "%s,%s", t, LISTENERPORT);	

	a->epid = t;
	a->epidr2 = t2;
	return 0;
}

//if match the internal cmd return 1,else return 0
static int 
_try_process_internal_cmd(pxy_agent_t *a, mcp_appbean_proto *p)
{
	retval r;
	switch(p->cmd) {
	case CMD_USER_VALID:
		a->user_id = p->user_id;
		return 1;
	case CMD_CLOSE_CONNECTION:
		worker_remove_agent(a);
		pxy_agent_close(a);
		return 1;
	case CMD_REFRESH_EPID:
		if(pbc_pattern_unpack(rpc_pat_retval, &p->content, &r) < 0) {
			E("unpack args error");
			return 1;
		}
		W("%s", r.option.buffer);
		worker_remove_agent(a);
		_refresh_agent_epid(a, &r.option);
		worker_insert_agent(a);
		return 1;
	}
	return 0;
}

static void get_send_data2(rec_msg_t *t, char* rval) 
{
	int hl = 21;
	int length = hl + t->body_len;
	int offset = hl;
	char padding = 0;
	int idx = 0;

#define __SB(b, d, l, idx)			\
	({					\
		char *__d = (char*)d;		\
		int j;				\
		for(j = 0;j < l;j++) {		\
			b[idx++] = __d[j];	\
		}				\
	})

	__SB(rval, &length, 2, idx);
	__SB(rval, &t->version, 1, idx);
	__SB(rval, &t->userid, 4, idx);
	__SB(rval, &t->cmd, 4, idx);
	__SB(rval, &t->seq, 2, idx);
	__SB(rval, &offset, 1, idx);
	__SB(rval, &t->format, 1, idx);
	__SB(rval, &t->compress, 1, idx);
	__SB(rval, &t->client_type, 2, idx);
	__SB(rval, &t->client_version, 2, idx);
	__SB(rval, &padding, 1, idx);
	
	// option use 0 template
	//set2buf(&rval, (char*)&padding, 4);

	if(t->body_len > 0) {
		memcpy(rval + idx, t->body, t->body_len);
	}

	char tmp[102400] = {0};
	to_hex(t->body, t->body_len, tmp);
	D("body is %s", tmp);
}

static int _send_to_client(rec_msg_t *m, pxy_agent_t *a)
{
	mrpc_buf_t *b = a->send_buf;
	size_t size_to_send = 21 + m->body_len;	//21 is the fixed header length

	D("agent fd %d, b->len %zu b->size %zu size_to send %d", 
	  a->fd, b->len, b->size, size_to_send);

	while(b->len - b->size < size_to_send) {
		if(mrpc_buf_enlarge(b) < 0) {
			E("buf enlarge error");
			return 0;
		}
	}

	get_send_data2(m, b->buf + b->offset);
	b->size += size_to_send;

	int r = mrpc_send2(a->send_buf, a->fd);
	if(r < 0) {
		I("send failed, prepare close!");
		return -1;
	}
	return 0;
}

void agent_mrpc_handler(mcp_appbean_proto *proto)
{
	rec_msg_t m;	
	_msg_from_proto(proto, &m);
	char ch[64] = {0};

	memcpy(ch, proto->epid.buffer, proto->epid.len);
	pxy_agent_t *a = map_search(&worker->root, ch);
	if(a == NULL) {
		W("cannot find the epid %s agent, cmd is %d", ch, m.cmd);
		return;
	}

	if(_try_process_internal_cmd(a, proto) > 0) {
		return;
	}

	if(_send_to_client(&m, a) < 0) {
		goto failed;
	}

	return ;

failed:
	//Add to reg3 rbtree
	//store_connection_context_reg3(agent);
	worker_remove_agent(a);
	pxy_agent_close(a);
	return;
}


char __url[32] = "tcp://10.10.41.9:7700";

static int agent_to_beans(pxy_agent_t *a, rec_msg_t* msg, int msp_unreg)
{
	UNUSED(a);
	char *url = NULL;
	get_app_url(msg->cmd, 1, NULL, NULL, NULL, &url);
	if(url == NULL) {
		D("cmd %d url is null", msg->cmd);
		return -1;
	}
	else {
		D("cmd %d url=%s", msg->cmd, url);
	}

	msg->uri = url;
//	msg->uri = __url;

	if(mrpc_us_send(msg) < 0) {
		E("mrpc send error");
		return -1;
	}

	free(url);
	return 0;
}

static int process_client_req(pxy_agent_t *agent)
{
	rec_msg_t msg;
	int r;
	while ((r = parse_client_data(agent, &msg)) > 0) {
		if(agent->user_id > 0) {
			if(msg.userid != agent->user_id) {
				W("user id %d not match with %d", 
				  msg.userid, agent->user_id);
				return -1;
			}
		}
		
		//when send failed, we should give a error response
		if(agent_to_beans(agent, &msg, 0) < 0) {
			if(msg.body_len > 0) {
				free(msg.body);
			}
			
			msg.body = BADGATEWAY;
			msg.body_len = sizeof(BADGATEWAY);
			msg.format = 128; //response
			
			if(_send_to_client(&msg, agent) < 0) {
				return -1;
			}

			continue;
		}

		if(msg.body_len > 0) {
			free(msg.body);
		}
		memset(&msg, 0, sizeof(msg));
	}
	
	return r;
}

void pxy_agent_close(pxy_agent_t *a)
{
	//TODO: refact needed
	//send flow to bean 105
	agent_send_netstat(a);
	agent_send_offstate(a);

	free(a->epid);
	a->epid = NULL;
	free(a->epidr2);
	a->epidr2 = NULL;
	a->user_ctx_len = 0;
	free(a->user_ctx);
	a->user_ctx = NULL;
	free(a->logintime);
	a->logintime = NULL;
	free(a->logouttime);
	a->logouttime = NULL;

	mrpc_buf_free(a->recv_buf);
	mrpc_buf_free(a->send_buf);

	if(a->fd > 0){
		/* close(agent->fd); */
		if(ev_del_file_item(worker->ev,a->ev) < 0){
			D("del file item err, errno is %d",errno);
		}

		D("close the socket");
		close(a->fd);
	}

	mp_free(worker->agent_pool,a);
}


pxy_agent_t * pxy_agent_new(mp_pool_t *pool,int fd,int userid)
{
	pxy_agent_t *agent = mp_alloc(pool);
	if(!agent){
		E("no mempry for agent"); 
		goto failed;
	}

	agent->send_buf = mrpc_buf_new(MSP_BUF_LEN);
	if(!agent->send_buf) {
		E("no mem for agent send_buf");
		goto failed;
	}
	agent->recv_buf = mrpc_buf_new(MSP_BUF_LEN);
	if(!agent->recv_buf) {
		mrpc_buf_free(agent->send_buf);
		E("no mem for agent recv_buf");
		goto failed;
	}

	agent->fd         = fd;
	agent->user_id    = userid;
	agent->epid = NULL;
	agent->user_ctx = NULL;
	agent->isreg = 0;
	agent->upflow = 0;
	agent->downflow = 0;
	return agent;

failed:
	return NULL;
}


int agent_send_netstat(pxy_agent_t *agent)
{
	if(agent->user_id<=0) {
		return 0;
	}

	I("user %d send netstat", agent->user_id);
	rec_msg_t msg = {0};
	msg.cmd = CMD_NET_STAT;
	msg.userid = agent->user_id;
	msg.logic_pool_id = agent->logic_pool_id;
	msg.epid = agent->epid;
	msg.user_context_len = agent->user_ctx_len;
	msg.user_context = agent->user_ctx;

	char tmp[128] = {0};
	struct pbc_slice s = {tmp, 128};

	mobile_flow_event_args args;
	memset(&args, 0, sizeof(args));
	args.upflow = agent->upflow;
	args.downflow = agent->downflow;
	
	int r = pbc_pattern_pack(rpc_pat_mobile_flow_event_args, 
				 &args, 
				 &s);
	if(r < 0) {
		E("pack args error");
		return -1;
	}

	msg.body_len = 128 -r;
	msg.body = tmp;
	agent_to_beans(agent, &msg, 0);
	return 0;
}

int agent_send_offstate(pxy_agent_t *agent)
{
	if(agent->user_id <= 0) {
		return 0;
	}

	rec_msg_t msg = {0};

	I("userid %d send offstate", agent->user_id);
	msg.cmd = CMD_CONNECTION_CLOSED;
	msg.userid = agent->user_id;
	msg.logic_pool_id = agent->logic_pool_id;
	msg.epid = agent->epid;
	msg.user_context_len = agent->user_ctx_len;
	msg.user_context = agent->user_ctx;

	agent_to_beans(agent, &msg, 0);
	return 0;
}


// client data
int agent_recv_client(ev_t *ev,ev_file_item_t *fi)
{
	UNUSED(ev);
	pxy_agent_t *agent = fi->data;

	int n = mrpc_recv2(agent->recv_buf,agent->fd);
	if(n == 0) {
		goto failed;
	}
	agent->upflow += n;

	if(process_client_req(agent) < 0) {
		I("prceoss client request failed");
		goto failed;
	}
	if(agent->recv_buf->offset == agent->recv_buf->size) {
		mrpc_buf_reset(agent->recv_buf);
	}
	return 0;

failed:
	I("operation failed, prepare close!");
	worker_remove_agent(agent);
	pxy_agent_close(agent);
	return -1;
}

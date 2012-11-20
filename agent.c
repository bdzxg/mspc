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
static char SERVERERROR[3] = {8, 244, 3};// 500
static char REQERROR[3] = {8, 144, 3};// 400
static char NOTEXIST[3] = {8, 155, 3};// 411

static int agent_to_beans(pxy_agent_t *a, rec_msg_t* msg, int msp_unreg);

int worker_insert_agent(pxy_agent_t *agent)
{
	int ret = 0;
	pthread_mutex_lock (&worker->mutex);
	ret = map_insert(&worker->root,agent);
	pthread_mutex_unlock(&worker->mutex);
	return ret;
}

void worker_remove_agent(pxy_agent_t *agent)
{
	if(agent->epid != NULL) {
		pthread_mutex_lock(&worker->mutex);	
		map_remove(&worker->root, agent->epid);
		pthread_mutex_unlock(&worker->mutex);
	}
}

void worker_insert_reg3(reg3_t* r3)
{
	pthread_mutex_lock(&worker->r3_mutex);
	map_insert_reg3(&worker->r3_root, r3);	
	pthread_mutex_unlock(&worker->r3_mutex);
}

reg3_t* worker_remove_reg3(char* key)
{
	pthread_mutex_lock(&worker->r3_mutex);
	reg3_t* r3 = map_remove_reg3(&worker->r3_root, key);
	pthread_mutex_unlock(&worker->r3_mutex);
	return r3;
}

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

void msp_send_req_to_bean(pxy_agent_t* a, int cmd, int bodylen, char* body)
{
	if(a->user_ctx != NULL) {
		rec_msg_t msg;
		msg.cmd = cmd;
		msg.userid = a->user_id;
		msg.logic_pool_id = a->logic_pool_id;
		msg.seq = 100000;
		msg.format = 0;
		msg.user_context_len = a->user_ctx_len;
		msg.user_context = a->user_ctx;
		msg.body_len = bodylen;
		msg.body = body;
		msg.epid = a->epid;
		msg.compress = 0;
		agent_to_beans(a, &msg, 1);
	}
}

void release_reg3(reg3_t* r3)
{
	free(r3->epid);
	free(r3->user_context);
	free(r3);
}


//TODO !!!!!!!
void send_to_client(pxy_agent_t* a, char* data, size_t len )
{
	mrpc_buf_t *b = a->send_buf;
	while(b->size - b->offset > len){
		mrpc_buf_enlarge(b);
	}

	int r = mrpc_send2(a->send_buf, a->fd);
	if(r < 0) {
		goto failed;
	}
	a->downflow += len;
	return ;

failed:
	W("send failed, prepare close!");
	//Add to reg3 rbtree
	//store_connection_context_reg3(agent);
	worker_remove_agent(a);
	pxy_agent_close(a);
	return;

}


int store_connection_context_reg3(pxy_agent_t *a)
{
	if(a->isreg == 1 && a->epid != NULL && a->user_ctx_len > 0)
	{
		// epid
		reg3_t* r3 = calloc(sizeof(reg3_t), 1);
		r3->epid = calloc(strlen(a->epid)+1, 1);
		memcpy(r3->epid, a->epid, strlen(a->epid));

		// context
		r3->user_context_len = a->user_ctx_len;
		r3->user_context = calloc(r3->user_context_len, 1);
		memcpy(r3->user_context, a->user_ctx, r3->user_context_len);

		r3->user_id = a->user_id;
		r3->logic_pool_id = a->logic_pool_id;

		r3->remove_time = time(NULL)+REG3_REMOVE_INTERVAL_MIN*60; 
		worker_insert_reg3(r3);

		// send 199 to appbean
		msp_send_req_to_bean(a, 199, 0, NULL);
		return 1;
	}
	else
		return 0;
}

int process_reg3_req(rec_msg_t* msg, pxy_agent_t* a)
{
	UNUSED(msg);
	UNUSED(a);
	return 1;
}

void send_client_flow_to_bean(pxy_agent_t *a)
{
	UNUSED(a);
	//TODO:
	return;

	/*MobileFlowEventArgs args;*/
	/*args.ClientType = a->clienttype;*/
	/*args.DownFlow = a->downflow;*/
	/*args.UpFlow = a->upflow;*/
	/*args.LoginTime.len = strlen(a->logintime);*/
	/*args.LoginTime.buffer = a->logintime;*/
	/*args.LogoffTime.buffer = get_now_time();*/
	/*args.LogoffTime.len = strlen(args.LogoffTime.buffer);*/

	/*rpc_pb_string str_input; */
	/*int inputsz = 1024;  */
	/*str_input.buffer = calloc(inputsz, 1);*/
	/*str_input.len = inputsz;*/
	/*rpc_pb_pattern_pack(rpc_pat_mobilefloweventargs, &args, &str_input);*/

	/*W("105 len %d", str_input.len);*/
	/*msp_send_req_to_bean(a, 105, str_input.len, str_input.buffer);*/
	/*free(str_input.buffer);*/
}

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
	D("b->offset is %d, msg->len %d", b->offset, msg->len);
	if(data_len < msg->len) {
		D("%lu < %lu, wait for more data",data_len, msg->len);
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
		msg->epid = agent->epid = generate_client_epid(msg->client_type, msg->client_version);
		agent->epidr2 = calloc(strlen(agent->epid)+strlen(LISTENERPORT)+2, 1);
		strncat(agent->epidr2, agent->epid, strlen(agent->epid));
		strncat(agent->epidr2, ",", 1);
		strncat(agent->epidr2, LISTENERPORT, strlen(LISTENERPORT));
		W("REG2 epid %s", agent->epidr2);
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

void agent_mrpc_handler(mcp_appbean_proto *proto)
{
	int size_to_send;
	rec_msg_t m;	
	_msg_from_proto(proto, &m);
	char ch[64] = {0};

	memcpy(ch, proto->epid.buffer, proto->epid.len);
	pxy_agent_t *a = map_search(&worker->root, ch);
	if(a == NULL) {
		E("cannot find the epid %s agent", m.epid);
		return;
	}

	mrpc_buf_t *b = a->send_buf;
	size_to_send = 21 + m.body_len;	//21 is the fixed header length
	D("agent fd %d, b->len %d b->size %d size_to send %d", 
	  a->fd, b->len, b->size, size_to_send);
	while(b->len - b->size < size_to_send) {
		if(mrpc_buf_enlarge(b) < 0) {
			E("buf enlarge error");
			return;
		}
	}
	get_send_data2(&m, b->buf + b->offset);
	b->size += size_to_send;

	int r = mrpc_send2(a->send_buf, a->fd);
	if(r < 0) {
		W("send failed, prepare close!");
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


void send_response_client(rec_msg_t* req,  pxy_agent_t* a, int code) 
{
	rec_msg_t rp_msg;
	rp_msg.cmd = req->cmd;
	rp_msg.body_len = 3;
	if(code == 504)
		rp_msg.body = OVERTIME;
	else if(code == 400)
		rp_msg.body = REQERROR;
	else if(code == 411)
		rp_msg.body = NOTEXIST;
	else 
		rp_msg.body = SERVERERROR;

	rp_msg.userid = req->userid;
	rp_msg.seq = req->seq;
	rp_msg.format = 128;
	rp_msg.compress = 0; 
	rp_msg.client_type = req->client_type;
	rp_msg.client_version = req->client_version;
	int len;
	char* d = get_send_data(&rp_msg, &len);
	D("fd %d, d %p, len %d", a->fd, d, len);
	send_to_client(a, d, (size_t)len);
	free(d);
}
 
char __url[32] = "tcp://10.10.10.184:7777";

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

//	msg->uri = url;
	msg->uri = __url;
	D("uri is %s", __url);
	if(mrpc_us_send(msg) < 0) {
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
		agent_to_beans(agent, &msg, 0);
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
	send_client_flow_to_bean(a);
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

// client data
void agent_recv_client(ev_t *ev,ev_file_item_t *fi)
{
	UNUSED(ev);
	pxy_agent_t *agent = fi->data;

	int n = mrpc_recv2(agent->recv_buf,agent->fd);
	if(n == 0) {
		goto failed;
	}
	agent->upflow += n;

	if(process_client_req(agent) < 0) {
		W("prceoss client request failed");
		goto failed;
	}
	if(agent->recv_buf->offset == agent->recv_buf->size) {
		mrpc_buf_reset(agent->recv_buf);
	}
	return;

failed:
	W("operation failed, prepare close!");
	//Add to reg3 rbtree
	//store_connection_context_reg3(agent);
	worker_remove_agent(agent);
	pxy_agent_close(agent);
	return;
}

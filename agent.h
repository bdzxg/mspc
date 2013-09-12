#ifndef _AGENT_H_
#define _AGENT_H_
#include "proxy.h"

typedef struct pxy_agent_s{
	int fd;
	int user_id;
	uint16_t bn_seq;
	char* epid;
	mrpc_buf_t *send_buf;
	mrpc_buf_t *recv_buf;
	struct rb_node rbnode;
	char* user_ctx;
	size_t user_ctx_len;
	int logic_pool_id;
	size_t isreg;
	int clienttype;
	long upflow;
	long downflow;
	char* logintime;
	char* logouttime;
	ev_time_item_t* timer;
        int isunreg;
        long last_active;
        int sid;
        char *ct;
        char *cv;
        struct hashtable *svr_req_buf; 
       // int svr_req_seq;
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


pxy_agent_t* pxy_agent_new(int,int, char*);
void pxy_agent_close(pxy_agent_t *);
int pxy_agent_data_received(pxy_agent_t *);
int pxy_agent_upstream(int ,pxy_agent_t *);
int pxy_agent_echo_test(pxy_agent_t *);
int pxy_agent_buffer_recycle(pxy_agent_t *);
int process_received_msg(size_t buf_size, uint8_t* buf_ptr, rec_msg_t* msg);
void agent_recv_client(ev_t *,ev_file_item_t*);
void agent_send_client(ev_t *,ev_file_item_t*);
void free_string_ptr(char* str);
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
int agent_mrpc_handler2(mcp_appbean_proto *proto, mrpc_connection_t *c,
                mrpc_message_t msg);

#define pxy_agent_for_each(agent,alist)				\
	list_for_each_entry((agent),list,&(alist)->list)	

#define pxy_agent_append(agent,alist)			\
	list_append(&(agent)->list,&(alist)->list)		

#define pxy_agent_remove(agent)			\
	list_remove(&(agent)->list)


static char* get_inner_cltype(char* client)
{
	if(!strcasecmp(client,"PC")) return "PCCL";
	if(!strcasecmp(client,"SMARTPHONE")) return "SMTP";
	if(!strcasecmp(client,"POCKETPC")) return "PCKT";
	if(!strcasecmp(client,"SYMBIAN")) return "SYMB";
	if(!strcasecmp(client,"J2ME")) return "JTME";
	if(!strcasecmp(client,"WAP")) return "WAPP";
	if(!strcasecmp(client,"WEB")) return "WEBB";
	if(!strcasecmp(client,"WV")) return "WVWV";
   	if(!strcasecmp(client, "POC")) return "POCC"; 
	if(!strcasecmp(client, "SYMBIANR1")) return "SYBA";
	if(!strcasecmp(client, "J2MER1")) return "JTEA";
	if(!strcasecmp(client, "LINUXMOTO")) return "LXMT";
	if(!strcasecmp(client, "SMS")) return "SMSS";
	if(!strcasecmp(client, "OMS")) return "OMSS";
	if(!strcasecmp(client, "IPHONE")) return "IPHO";
	if(!strcasecmp(client, "ANDROID")) return "ANDR";
	if(!strcasecmp(client, "MTKVRE")) return "MKVR";
	if(!strcasecmp(client, "MTKMYTHROAD")) return "MKMY";
	if(!strcasecmp(client, "MTKDALOADER")) return "MKDA";
	if(!strcasecmp(client, "MTKMORE")) return "MKMO";
	if(!strcasecmp(client, "ROBOT")) return "ROBT";
	if(!strcasecmp(client, "BLACKBERRY")) return "BKBR";
	if(!strcasecmp(client, "PCGAME")) return "PCGM";
	if(!strcasecmp(client, "WEBGAME")) return "WBGM";
	if(!strcasecmp(client, "PCMUSIC")) return "PCMS";
	if(!strcasecmp(client, "PCSMART")) return "PCSM";
	if(!strcasecmp(client, "WEBBAR")) return "WBAR";
	if(!strcasecmp(client, "IPAD")) return "IPAD";
	if(!strcasecmp(client, "WEBUNION")) return "WUNION";
	if(!strcasecmp(client, "MTKMEX")) return "MKMX";
	if(!strcasecmp(client, "MTKSXM")) return "MKSM";
	if(!strcasecmp(client, "MTK")) return "MTKK";
	if(!strcasecmp(client, "EANDROID")) return "EAND";
	if(!strcasecmp(client, "ESYMBIAN")) return "ESYM";
	if(!strcasecmp(client, "EIPHONE")) return "EIPH";
	if(!strcasecmp(client, "WINPHONE")) return "WPHO";
	if(!strcasecmp(client, "WIN8")) return "WIN8";
	if(!strcasecmp(client, "PCMAC")) return "PCMC";
	if(!strcasecmp(client, "SMSDEP")) return "SMSD";
	if(!strcasecmp(client, "SMSBP")) return "SMSB";
	if(!strcasecmp(client, "LAUNCHER")) return "LACH";

	return "ANDR";
}

static inline char* get_client_type(int num)
{
	switch(num)
	{
	case 1: return "PC";
	case 2: return "SMARTPHONE"; 
	case 3: return "POCKETPC";
	case 4: return "SYMBIAN";
	case 5: return "J2ME"; 
	case 6: return "WAP"; 
	case 7: return "WEB"; 
	case 8: return "WV";
	case 9: return "POC"; 
	case 10: return "SYMBIANR1"; 
	case 11: return "J2MER1"; 
	case 12: return "LINUXMOTO"; 
	case 13: return "SMS"; 
	case 14: return "OMS"; 
	case 15: return "IPHONE"; 
	case 16: return "ANDROID"; 
	case 17: return "MTKVRE"; 
	case 18: return "MTKMYTHROAD"; 
	case 19: return "MTKDALOADER"; 
	case 20: return "MTKMORE"; 
	case 21: return "ROBOT"; 
	case 22: return "BLACKBERRY"; 
	case 23: return "PCGAME"; 
	case 24: return "WEBGAME"; 
	case 25: return "PCMUSIC"; 
	case 26: return "PCSMART"; 
	case 27: return "WEBBAR"; 
	case 28: return "IPAD"; 
	case 29: return "WEBUNION"; 
	case 30: return "MTKMEX"; 
	case 31: return "MTKSXM"; 
	case 32: return "MTK"; 
	case 33: return "EANDROID"; 
	case 34: return "ESYMBIAN"; 
	case 35: return "EIPHONE"; 
	case 36: return "WINPHONE"; 
	case 37: return "WIN8"; 
	case 38: return "PCMAC"; 
	case 39: return "SMSDEP"; 
	case 40: return "SMSBP"; 
        case 41: return "LAUNCHER";
	}
	return "ANDROID";
}
//static inline char* get_client_version(int version)
//{
//	switch(version) {
//	case 1: return "0100";
//	case 2: return "0101";
//	case 3: return "0102";
//	case 4: return "0103";
//	case 5: return "0104";
//	case 6: return "0105";																	         
//	case 7: return "0106";
//	case 8: return "0107";
//	case 9: return "0108";
//	case 10: return "0109";
//				 
//	case 11: return "0200";
//	case 12: return "0201";
//	case 13: return "0202";
//	case 14: return "0203";
//	case 15: return "0204";
//	case 16: return "0205";
//	case 17: return "0206";
//	case 18: return "0207";
//	case 19: return "0208";
//	case 20: return "0209";
//				 
//	case 21: return "0300";
//	case 22: return "0301";
//	case 23: return "0302";
//	case 24: return "0303";
//	case 25: return "0304";
//	case 26: return "0305";
//	case 27: return "0306";
//	case 28: return "0307";
//	case 29: return "0308";
//	case 30: return "0309";
//				 
//	case 31: return "0400";
//	case 32: return "0401";
//	case 33: return "0402";
//	case 34: return "0403";
//	case 35: return "0404";
//	case 36: return "0405";
//	case 37: return "0406";
//	case 38: return "0407";
//	case 39: return "0408";
//	case 40: return "0409";
//	}
//	return "0100";
//}

static inline char* get_client_version(int version)
{
	switch(version) {
	case 1: return "0500";
	case 2: return "0501";
	case 3: return "0502";
	case 4: return "0503";
	case 5: return "0504";
	case 6: return "0505";
	case 7: return "0506";
	case 8: return "0507";
	case 9: return "0508";
	case 10: return "0509";

        case 100: return "0100";
        case 101: return "0101";
        case 102: return "0102";
        case 103: return "0103";
        case 104: return "0104";
        case 105: return "0105";
        case 106: return "0106";
        case 107: return "0107";
        case 108: return "0108";
        case 109: return "0109";

        case 200: return "0200";
        case 201: return "0201";
        case 202: return "0202";
        case 203: return "0203";
        case 204: return "0204";
        case 205: return "0205";
        case 206: return "0206";
        case 207: return "0207";
        case 208: return "0208";
        case 209: return "0209";
				 
//	case 11: return "0600";
//	case 12: return "0601";
//	case 13: return "0602";
//	case 14: return "0603";
//	case 15: return "0604";
//	case 16: return "0605";
//	case 17: return "0606";
//	case 18: return "0607";
//	case 19: return "0608";
//	case 20: return "0609";
//				 
//	case 21: return "0700";
//	case 22: return "0701";
//	case 23: return "0702";
//	case 24: return "0703";
//	case 25: return "0704";
//	case 26: return "0705";
//	case 27: return "0706";
//	case 28: return "0707";
//	case 29: return "0708";
//	case 30: return "0709";
//				 
//	case 31: return "0800";
//	case 32: return "0801";
//	case 33: return "0802";
//	case 34: return "0803";
//	case 35: return "0804";
//	case 36: return "0805";
//	case 37: return "0806";
//	case 38: return "0807";
//	case 39: return "0808";
//	case 40: return "0809";
	}
	return "0500";
}

static inline char* generate_client_epid(int type, int version)
{
	char* cl_type = get_client_type(type);
	char* cl_version = get_client_version(version);
	struct timeval tv;
	time_t timesec;
	struct tm *p;
	time(&timesec);
	p = gmtime(&timesec);
	gettimeofday(&tv, NULL);
	char* t = calloc(20, 1);	
	if(!t) {
		return NULL;
	}
	sprintf(t, "%02d%02d%lu", p->tm_min, p->tm_sec, tv.tv_usec);
	char* cltype = get_inner_cltype(cl_type);
	char* epid = calloc(strlen(t)+ strlen(cltype) + strlen(cl_version) + 1, 1);
	if(!epid) {
		return NULL;
	}
	sprintf(epid, "%s%s%s", cltype,  cl_version, t);
	free(t);
	return epid;
}

#endif

#ifndef _proto_h_
#define _proto_h_
typedef struct rec_message_s{
	int len;
	char version;
	int userid;
	int cmd;
	uint16_t seq;
	int off;
	int format;	
	int compress;
	uint16_t client_type;
	uint16_t client_version;
	int logic_pool_id;
	char *option;
	size_t option_len;
	char *body;
	size_t body_len;
	char *user_context;
	size_t user_context_len;
	char *epid;
	list_head_t head;
	char *uri; //FIXME
}rec_msg_t;
#endif

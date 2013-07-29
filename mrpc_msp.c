#include "proxy.h"
#include "mrpc.h"
#include "mrpc_msp.h"

char __resp[9] = {0x0a, 0x07, 0x4d, 0x43, 0x50, 0x2f, 0x33, 0x2e, 0x30};
static char* PROTOCOL = "MCP/3.0";
static char __compact_uri[64] = {0};

static char* get_compact_uri(rec_msg_t* msg)
{
	sprintf(__compact_uri, "id:%d;p=%d;", msg->userid,msg->logic_pool_id);	
	return __compact_uri;
}

static void get_rpc_arg(mcp_appbean_proto* args, rec_msg_t* msg)
{
	args->protocol.len = strlen(PROTOCOL);
	args->protocol.buffer = PROTOCOL;
	args->cmd = msg->cmd;
	char* uri = get_compact_uri(msg);
	args->compact_uri.len = strlen(uri);
	args->compact_uri.buffer = uri;
	args->user_id = msg->userid;
	args->sequence = msg->seq;
	args->opt = msg->format;

	if (msg->user_context_len > 0) {
		args->user_context.len = msg->user_context_len;
		args->user_context.buffer = msg->user_context;
	} else { 
		args->user_context.len = 0;
		args->user_context.buffer = NULL;
	}

	D("request user_context.len %d, body_len %zu", args->user_context.len, 
                        msg->body_len);
	if (msg->body_len > 0) {	
		args->content.len = msg->body_len;
		args->content.buffer = msg->body;
	} else {
		args->content.len = 0;
		args->content.buffer = NULL;
	}

	args->epid.len = strlen(msg->epid);
	args->epid.buffer = msg->epid;
	args->zip_flag = msg->compress;
}	

//static int _svr_send(rec_msg_t *msg, mrpc_connection_t *c)
//{
//	mcp_appbean_proto body;
//	get_rpc_arg(&body, msg);
//
//	size_t buf_len = body.content.len + 1024;
//	char *body_buf = malloc(buf_len);
//	if (!body_buf) {
//		E("cannot malloc body_buf");
//		return -1;
//	}
//	struct pbc_slice s = { body_buf, buf_len };
//	D("the buf size is %lu", buf_len);
//	int r =	pbc_pattern_pack(rpc_pat_mcp_appbean_proto, &body, &s);
//	if (r < 0) {
//		E("pack body error");
//		goto failed;
//	}
//
//	int body_len = buf_len - r;
//	D("body_len is %d", body_len);
//	mrpc_request_header header;
//	memset(&header, 0, sizeof(header));
//	header.sequence = ++c->seq;
//	header.body_length = body_len + 1; // by protocol we need plus 1
//
//	char header_buf[128];
//	struct pbc_slice s2 = {header_buf, 128};
//	r = pbc_pattern_pack(rpc_pat_mrpc_request_header, &header, &s2);
//	if (r < 0) {
//		E("pack header error");
//		goto failed;
//	}
//	int head_len = 128 - r;
//	size_t pkg_len = 12 + body_len + head_len;
//	while(c->send_buf->len - c->send_buf->size < pkg_len) {
//		if (mrpc_buf_enlarge(c->send_buf) < 0) {
//			E("enlarge error");
//			goto failed;
//		}
//	}
//
//	*(uint32_t*)(c->send_buf->buf + c->send_buf->size) = htonl(0x0badbee0);
//	c->send_buf->size += 4;
//	*(uint32_t*)(c->send_buf->buf + c->send_buf->size) = htonl(pkg_len);
//	c->send_buf->size += 4;
//	*(uint16_t*)(c->send_buf->buf + c->send_buf->size) = htons((short)head_len);
//	c->send_buf->size += 2;
//	*(uint16_t*)(c->send_buf->buf + c->send_buf->size) = 0;
//	c->send_buf->size += 2;
//
//	memcpy(c->send_buf->buf + c->send_buf->size, header_buf, head_len);
//	c->send_buf->size += head_len;
//	
//	memcpy(c->send_buf->buf + c->send_buf->size, body_buf, body_len);
//	c->send_buf->size += body_len;
//
//	if (mrpc_send(c) < 0) {
//		goto failed;
//	}
//	free(body_buf);
//	return 0;
//failed:
//	free(body_buf);
//	return -1;
//}
//
static inline 
void mrpc_resp_from_req(mrpc_message_t *req, mrpc_message_t *resp)
{
	resp->mask = htonl(0xBADBEE01);
	resp->h.resp_head.sequence = req->h.req_head.sequence;
	D("the resp seq is %d-%d",req->h.req_head.sequence,
	  resp->h.resp_head.sequence);
	resp->h.resp_head.to_id = req->h.req_head.to_id;
	resp->h.resp_head.from_id = req->h.req_head.from_id;
	resp->h.resp_head.opt = 0;
	resp->h.resp_head.body_length = 0;
}

int send_svr_response(mrpc_req_buf_t *svr_req_data, rec_msg_t *client_msg)
{
        mcp_appbean_proto body;
        get_rpc_arg(&body, client_msg);
        body.ip.buffer = NULL;
        body.ip.len = 0;
        body.category_name.buffer = NULL;
        body.category_name.len = 0;
        size_t buf_len = body.content.len + 1024;
        char *body_buf = malloc(buf_len);
        if (!body_buf) {
                E("malloc body_buf error!");
                return -1;
        }
        struct pbc_slice s = { body_buf, buf_len };
        int r1 = pbc_pattern_pack(rpc_pat_mcp_appbean_proto, &body, &s);
        if (r1 < 0) {
                E("pack body error!");
                goto failed;
        }

        int body_len = buf_len - r1;
        
        mrpc_message_t resp;
        mrpc_buf_t *sbuf = svr_req_data->c->send_buf;
	mrpc_resp_from_req(&svr_req_data->msg, &resp);
	resp.h.resp_head.response_code = 200;
	resp.h.resp_head.body_length = body_len + 1;

	char temp[32];
	struct pbc_slice obuf = {temp, 32};
	int r = pbc_pattern_pack(rpc_pat_mrpc_response_header, &resp.h.resp_head, &obuf);
	if (r < 0) {
		E("rpc pack response header error");
                return -1;
	}

	uint32_t resp_size = 12 + 32 - r + body_len ;
	while(svr_req_data->c->send_buf->size + resp_size > 
                        svr_req_data->c->send_buf->len) {
		mrpc_buf_enlarge(svr_req_data->c->send_buf);
	}

	//write the response to the send_buf
	*((uint32_t*)(sbuf->buf + sbuf->size)) = htonl(0X0BADBEE1);
	sbuf->size += 4;
	*((uint32_t*)(sbuf->buf + sbuf->size)) = htonl(resp_size);
	sbuf->size += 4;
	*((uint32_t*)(sbuf->buf + sbuf->size)) = htons(32- r);
	sbuf->size += 2;
	*((uint32_t*)(sbuf->buf + sbuf->size)) = htons(0);
	sbuf->size += 2;
	memcpy(sbuf->buf + sbuf->size, temp, 32 - r);
	sbuf->size += 32 - r;
	memcpy(sbuf->buf + sbuf->size, body_buf, body_len);
	sbuf->size += body_len;
	
	int n = mrpc_send(svr_req_data->c);
	if (n < 0) {
                goto failed;
	}

        free(body_buf);
	return 1;
failed:
        free(body_buf);
        return -1;
}

int send_direct_response(mrpc_connection_t *c, mrpc_message_t msg)
{
	mrpc_message_t resp;
	mrpc_buf_t *sbuf = c->send_buf;
	mrpc_resp_from_req(&msg, &resp);
	resp.h.resp_head.response_code = 200;
	resp.h.resp_head.body_length = 10;

	char temp[32];
	struct pbc_slice obuf = {temp, 32};
	int r = pbc_pattern_pack(rpc_pat_mrpc_response_header, &resp.h.resp_head, &obuf);
	if (r < 0) {
		E("rpc pack response header error");
                return -1;
	}
	uint32_t resp_size = 12 + 32 - r + sizeof(__resp);
	while(c->send_buf->size + resp_size > c->send_buf->len) {
		mrpc_buf_enlarge(c->send_buf);
	}

	//write the response to the send_buf
	*((uint32_t*)(sbuf->buf + sbuf->size)) = htonl(0X0BADBEE1);
	sbuf->size += 4;
	*((uint32_t*)(sbuf->buf + sbuf->size)) = htonl(resp_size);
	sbuf->size += 4;
	*((uint32_t*)(sbuf->buf + sbuf->size)) = htons(32- r);
	sbuf->size += 2;
	*((uint32_t*)(sbuf->buf + sbuf->size)) = htons(0);
	sbuf->size += 2;
	memcpy(sbuf->buf + sbuf->size, temp, 32 - r);
	sbuf->size += 32 - r;
	memcpy(sbuf->buf + sbuf->size, __resp, 9);
	sbuf->size += 9;
	
	int n = mrpc_send(c);
	if (n < 0) {
                return -1;
	}

	return 1;
}

int mrpc_process_client_req(mrpc_connection_t *c)
{
	mrpc_message_t msg;
	mrpc_buf_t *b = c->recv_buf;
	mcp_appbean_proto proto;

	int r = mrpc_parse(b, &msg, &proto);
	
	if (r < 0) {
		goto failed;
	}

	if (r == 0) {
		return 0;
	}

        //char *method_name;
        int backtrack = 0;

        if (msg.mask == 0x0badbee0) {
                struct pbc_slice slice;
                slice = msg.h.req_head.to_method;
                if (strncmp(slice.buffer, "BacktrackMethod", slice.len) == 0) {
                        backtrack = 1;
                }
                
                // manual set backtrack method for test;
//                backtrack = 1;
        }        	

        //send the response
        if (backtrack == 0 ) {
                if (send_direct_response(c, msg) < 0 ) goto failed;
                agent_mrpc_handler(&proto);
        } else {
                agent_mrpc_handler2(&proto, c, msg);
        }

        //reset the recv buf, send_buf will be reset by _send function
        if (b->offset == b->size) {
                mrpc_buf_reset(b);
        }
        
        return 1;

failed:
	return -1;
}

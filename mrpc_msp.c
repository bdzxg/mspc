#include "proxy.h"
#include "mrpc.h"

char __resp[9] = {0x0a, 0x07, 0x4d, 0x43, 0x50, 0x2f, 0x33, 0x2e, 0x30};

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

int send_svr_response(mrpc_connection_t *c, rec_msg_t *msg)
{
        // be careful memleak 
        
}

int send_response(mrpc_connection_t *c, mrpc_message_t msg, mcp_appbean_proto proto,
                mrpc_buf_t *b)
{
	mrpc_message_t resp;
	mrpc_buf_t *sbuf = c->send_buf;
	mrpc_resp_from_req(&msg, &resp);
	resp.h.resp_head.response_code = 200;
	resp.h.resp_head.body_length = 10;

	char temp[32];
	struct pbc_slice obuf = {temp, 32};
	int r = pbc_pattern_pack(rpc_pat_mrpc_response_header, &resp.h.resp_head, &obuf);
	if(r < 0) {
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
	
	if(r < 0) {
		goto failed;
	}

	if(r == 0) {
		return 0;
	}

        char *method_name;
        int backtrack = 0;

        if (msg.mask == 0x0badbee0) {
                struct pbc_slice slice;
                slice = msg.h.req_head.to_method;
                if (strncmp(slice.buffer, "BacktrackMethod", slice.len) == 0) {
                        backtrack = 1;
                }
        }

        backtrack = 1;
	//send the response
        if (backtrack == 0 ) {
                if (send_response(c, msg, proto, b) < 0 ) goto failed;
                agent_mrpc_handler(&proto);

                //reset the recv buf, send_buf will be reset by _send function
                if (b->offset == b->size) {
                        mrpc_buf_reset(b);
                }

                return 1;
        } else {
                agent_mrpc_handler2(&proto, c);
        }

failed:
	return -1;
}



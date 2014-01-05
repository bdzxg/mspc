#include "proxy.h"
#include "agent.h"
#include <pthread.h>
#include "route.h"
#include "mrpc_msp.h"
#include "hashtable_itr.h"
#include "datalog.h"

extern pxy_worker_t *worker;
extern pxy_settings setting;
extern upstream_map_t *upstream_root;
size_t packet_len;

#define MSP_BUF_LEN 1024

static char OVERTIME[3] = {8, 248, 3};// 504
static char BADGATEWAY[3] = {8, 246, 3};// 502
static char SERVERERROR[3] = {8, 244, 3};// 500
//static char REQERROR[3] = {8, 144, 3};// 400
static char AUTHERROR[3] = {8, 145, 3};// 400
//static char NOTEXIST[3] = {8, 155, 3};// 411
static int agent_to_beans(pxy_agent_t *, rec_msg_t*, int);
static int agent_send_netstat(pxy_agent_t *);
static int agent_send_offstate(pxy_agent_t *agent);
void agent_timer_handler(ev_t* ev, ev_time_item_t* ti);
static int _send_to_client(rec_msg_t *m, pxy_agent_t *a);

int worker_insert_agent(pxy_agent_t *agent)
{
        return map_insert(&worker->root,agent);
}

void worker_remove_agent(pxy_agent_t *agent)
{
        if (agent->epid != NULL) {
                map_remove(&worker->root, agent->epid);
        }
}

void worker_insert_reg3(reg3_t* r3)
{
        map_insert_reg3(&worker->r3_root, r3);  
}

void log_mcp_message(pxy_agent_t *a, rec_msg_t *msg, int direction)
{
        return;
        char body[150000] = {0};
        to_hex(msg->body, msg->body_len, body);
        char *to_client = "Send";
        char *to_server = "Receive";
        char *dir = NULL;

        if (!direction) dir = to_server;
        else dir = to_client;

        I("%s MCP/3.0 %s: len %d, version %d, uid %d, cmd %d, seq %u, offset %d, "
                        "format %d, cmp %d, ct %u, cv %u, opt (null), epid %s, body %s", dir, 
                        msg->format == 0 ? "request" : "response", msg->len, msg->version, 
                        msg->userid, msg->cmd, msg->seq, msg->off, msg->format, msg->compress, 
                        msg->client_type, msg->client_version, a->epid, body);
}

//ok 1, not finish 0, error -1
int parse_client_data(pxy_agent_t *agent, rec_msg_t* msg)
{
        mrpc_buf_t *b = agent->recv_buf;
#define __B (b->buf + b->offset)
#define __B1 (b->buf + b->offset++)
        size_t data_len = b->size - b->offset;

        if (data_len < 2) {
                D("length %lu is not enough", data_len);
                return 0;
        }

        msg->len = (int)le16toh(*(uint16_t*)__B);
        D("b->offset is %zu, msg->len %d", b->offset, msg->len);
        if (data_len < msg->len) {
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

        b->offset ++;   //oimt the empty byte
        msg->option_len = msg->off - 21; //21 is the fixed length of header
        b->offset += msg->option_len;

        msg->body_len = msg->len - msg->off;
        D("body_len %zu", msg->body_len);

        if (agent->user_ctx_len == 0) {
                msg->user_context_len = 0; 
                msg->user_context = NULL;
        } else {
                msg->user_context_len = agent->user_ctx_len;
                msg->user_context = agent->user_ctx;
        }

        if (msg->body_len >0) {
                msg->body = malloc(msg->body_len);
                if (msg->body == NULL)
                        return -1;

                memcpy(msg->body, b->buf + b->offset,msg->body_len);
        } else {
                msg->body = NULL;
                msg->body_len = 0; 
        }
        b->offset += msg->body_len;

        if (agent->epid == NULL) {
                agent->epid = generate_client_epid(msg->client_type,
                                msg->client_version);
                if (!agent->epid) {
                        E("gen epid error fd %d, uid %d, cmd %d", agent->fd,
                                        msg->userid, msg->cmd);
                        goto ERROR_RESPONSE;
                }
                I("fill epid fd %d, cmd %d, uid %d, epid %s", agent->fd, msg->cmd,
                                msg->userid, agent->epid);
                agent->clienttype = msg->client_type;
                if (worker_insert_agent(agent) < 0) {
                        E("insert agent error, epid %s",agent->epid);
                        goto ERROR_RESPONSE;
                }
        }
        msg->epid = agent->epid;

        //user context
        if (agent->user_ctx != NULL && agent->user_ctx_len > 0) {
                msg->user_context = agent->user_ctx;
                msg->user_context_len = agent->user_ctx_len;
        }

        D("msp->epid %s", msg->epid);
        msg->logic_pool_id = agent->logic_pool_id;
        log_mcp_message(agent, msg, 0);
        return 1;

ERROR_RESPONSE:
        msg->body = SERVERERROR;
        msg->body_len = sizeof(SERVERERROR);
        msg->format = 128;
        _send_to_client(msg, agent);
        return -1;
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

        if (p->content.len > 0) {
                m->body_len = p->content.len;
                m->body = p->content.buffer;
        }
}

#define CMD_USER_VALID 101
#define CMD_CLOSE_CONNECTION 102
#define CMD_CONNECTION_CLOSED 103
#define CMD_NET_STAT 104
#define CMD_USER_UNREG 10105

//if match the internal cmd return 1,else return 0
        static int 
_try_process_internal_cmd(pxy_agent_t *a, mcp_appbean_proto *p)
{
        //        retval r; 
        switch(p->cmd) {
                case CMD_USER_VALID:
                        a->user_id = p->user_id;
                        size_t l = p->user_context.len;
                        if (l <= 0) return 1;

                        free(a->user_ctx);
                        a->user_ctx = malloc(l);
                        if (!a->user_ctx) {
                                W("cannot malloc for usercontext uid %d, epid %s",
                                                p->user_id, a->epid);
                                return 1;
                        }

                        a->user_ctx_len = l;
                        memcpy(a->user_ctx, p->user_context.buffer, l);
                        I("save %zu length ctx", l);

                        struct pbc_slice slice = {a->user_ctx, a->user_ctx_len}; 
                        user_context user_ctx; 
                        int r1 = pbc_pattern_unpack(rpc_pat_user_context, 
                                        &slice, &user_ctx);
                        if (r1 < 0) {
                                E("unpack user context failed! uid %d, epid %s",
                                                p->user_id, a->epid);
                                return 1;
                        } 

                        int ct_len = user_ctx.client_platform.len;
                        if (ct_len > 0) {
                                a->ct = calloc(1, ct_len + 1);
                                strncpy(a->ct, user_ctx.client_platform.buffer, 
                                                ct_len);
                        }

                        int cv_len = user_ctx.client_version.len;
                        if (cv_len > 0) {
                                a->cv = calloc(1, cv_len + 1);
                                strncpy(a->cv, user_ctx.client_version.buffer, cv_len);
                        }

                        a->sid = user_ctx.sid;
                        I("unpack ctx sid %d, uid %d, cv %s, cp %s, epid %s", 
                                        user_ctx.sid, user_ctx.user_id, a->cv, a->ct, a->epid); 

                        return 1;
                case CMD_CLOSE_CONNECTION:
                        worker_remove_agent(a);
                        pxy_agent_close(a);
                        return 1;

                        //      remove reg3 logic
                        //      case CMD_REFRESH_EPID:
                        //              if (pbc_pattern_unpack(rpc_pat_retval, &p->content, &r) < 0) {
                        //                      W("unpack args error");
                        //                      return 1;
                        //              }
                        //              worker_remove_agent(a);
                        //              _refresh_agent_epid(a, &r.option);
                        //              worker_insert_agent(a);
                        //              return 1;
        }

        return 0;
}

static void get_send_data2(rec_msg_t *t, char* rval, pxy_agent_t *a)
{
        int hl = 21;
        int length = hl + t->body_len;
        t->len = length;
        t->off = hl;
        int offset = hl;
        char padding = 0;
        int idx = 0;

#define __SB(b, d, l, idx)                      \
        ({                                      \
         char *__d = (char*)d;          \
         int j;                         \
         for(j = 0;j < l;j++) {         \
         b[idx++] = __d[j];     \
         }                              \
         })

        __SB(rval, &length, 2, idx);
        __SB(rval, &t->version, 1, idx);
        __SB(rval, &t->userid, 4, idx);
        __SB(rval, &t->cmd, 4, idx);
        if (t->format < 128) {
                __SB(rval, &a->bn_seq, 2, idx);
                //++a->bn_seq;
        } else {
                __SB(rval, &t->seq, 2, idx);
        }
        __SB(rval, &offset, 1, idx);
        __SB(rval, &t->format, 1, idx);
        __SB(rval, &t->compress, 1, idx);
        __SB(rval, &t->client_type, 2, idx);
        __SB(rval, &t->client_version, 2, idx);
        __SB(rval, &padding, 1, idx);

        // option use 0 template
        //set2buf(&rval, (char*)&padding, 4);

        if (t->body_len > 0) {
                memcpy(rval + idx, t->body, t->body_len);
        }

        //      char tmp[102400] = {0};
        //      to_hex(t->body, t->body_len, tmp);
        //      D("body is %s", tmp);
        log_mcp_message(a, t, 1);
}

static int _send_to_client(rec_msg_t *m, pxy_agent_t *a)
{
        mrpc_buf_t *b = a->send_buf;
        size_t size_to_send = 21 + m->body_len; //21 is the fixed header length

        D("agent fd %d, b->len %zu b->size %zu size_to send %zu", 
                        a->fd, b->len, b->size, size_to_send);

        while(b->len - b->size < size_to_send) {
                if (mrpc_buf_enlarge(b) < 0) {
                        W("buf enlarge error");
                        return 0;
                }
        }

        get_send_data2(m, b->buf + b->size, a);
        b->size += size_to_send;

        int r = mrpc_send2(a->send_buf, a->fd);
        if (r < 0) {
                I("cmd %d send to uid %d failed, prepare close!",
                                m->cmd, a->user_id);
                return -1;
        }
        I("cmd %d succ sent to uid %d, epid %s, fd %d", m->cmd, a->user_id, 
                        a->epid, a->fd);
        return 0;
}

uint16_t seq_increment(uint16_t seq)
{
        D("seq = %u", seq);
        if (seq == 32767) 
                return 1;
        else 
                return ++seq;
}

void agent_mrpc_handler(mcp_appbean_proto *proto)
{
        rec_msg_t m;    
        _msg_from_proto(proto, &m);
        char ch[64] = {0};

        if (proto->epid.len > sizeof(ch)) {
                char *badid = calloc(1, proto->epid.len + 1);
                memcpy(badid, proto->epid.buffer, proto->epid.len);
                W("bad epid cmd %d, seq %d, opt %d, userid %d, epid %s",
                                proto->cmd, proto->sequence, proto->opt, proto->user_id, ch);
                free(badid);
                return;
        }

        memcpy(ch, proto->epid.buffer, proto->epid.len);
        I("receive backend cmd %d, seq %d, opt %d, userid %d, epid %s",
                        proto->cmd, proto->sequence, proto->opt, proto->user_id, ch);

        pxy_agent_t *a = map_search(&worker->root, ch);
        if (a == NULL) {
                I("cannot find the epid %s agent, uid %d, cmd is %d", ch, 
                                m.userid, m.cmd);
                return;
        }

        if (_try_process_internal_cmd(a, proto) > 0) {
                return;
        }

        if (m.format < 128) 
                a->bn_seq = seq_increment(a->bn_seq);

        if (_send_to_client(&m, a) < 0) {
                goto failed;
        }

        return;

failed:
        worker_remove_agent(a);
        pxy_agent_close(a);
        return;
}

int agent_mrpc_handler2(mcp_appbean_proto *proto, mrpc_connection_t *c,
                mrpc_message_t msg)
{
        rec_msg_t m;    
        _msg_from_proto(proto, &m);
        char ch[64] = {0};

        if (proto->epid.len > sizeof(ch)) {
                char *badid = calloc(1, proto->epid.len + 1);
                memcpy(badid, proto->epid.buffer, proto->epid.len);
                W( "bad epid cmd %d, seq %d, opt %d, userid %d, epid %s",
                                proto->cmd, proto->sequence, proto->opt, proto->user_id, ch);
                free(badid);
                return -1;
        }

        memcpy(ch, proto->epid.buffer, proto->epid.len);
        I("receive server request cmd %d seq %d opt %d userid %d epid %s",
                        proto->cmd, proto->sequence, proto->opt, proto->user_id, ch);

        pxy_agent_t *a = map_search(&worker->root, ch);
        if (a == NULL) {
                I("cannot find the epid %s agent, cmd is %d", ch, m.cmd);
                return -1;
        }

        if (_try_process_internal_cmd(a, proto) > 0) {
                return 1;
        }

        struct hashtable *table = a->svr_req_buf;
        mrpc_req_buf_t *svr_req_data = calloc(1, sizeof(*svr_req_data));
        svr_req_data->c = c;
        c->refs += 1;
        svr_req_data->msg = msg;
        svr_req_data->userid = m.userid;
        svr_req_data->cmd = m.cmd;
        svr_req_data->format = m.format;
        svr_req_data->compress = m.compress;
        svr_req_data->time = time(NULL) + setting.transaction_timeout;
        a->bn_seq = seq_increment(a->bn_seq);
        proto->sequence = a->bn_seq; 
        svr_req_data->sequence = a->bn_seq;

        int r = hashtable_insert(table, &(svr_req_data->sequence), svr_req_data);
        I("server request insert buffer seq %d, cmd %d, data=%p, table=%p", proto->sequence,
                        proto->cmd, svr_req_data, table);

        if (r != -1) {
                E("server request table insert error!");
                return -1;  
        }

        if (_send_to_client(&m, a) < 0) {
                goto failed;
        }

        return 1;
failed:
        worker_remove_agent(a);
        pxy_agent_close(a);
        return -1;
}

static void agent_svr_response(pxy_agent_t *a, rec_msg_t *msg, 
                struct hashtable *table, mrpc_req_buf_t *svr_req_data)
{
        int is_removed = 1;
        int key = msg->seq;
        //if (NULL == hashtable_remove(table, &(msg->seq))) {
        if (NULL == hashtable_remove(table, &key)) {
                E("remove server request table item error!");                
                is_removed = 0;
        }

        // be attention to connection lost
        if (svr_req_data->c->conn_status == MRPC_CONN_CONNECTED
                        && send_svr_response(svr_req_data, msg) > 0) {
                I("agent send server response OK! fd %d,  uid %d, epid %s, cmd %d",
                                a->fd, a->user_id, a->epid, msg->cmd); 
        } else {
                E("agent send server response ERROR! fd %d, uid %d, epid %s, "
                                "cmd %d", a->fd, a->user_id, a->epid, msg->cmd);
        }

        if (is_removed) {
                svr_req_data->c->refs -= 1;
                free(svr_req_data);
        }
}

char __url[32] = "tcp://10.10.41.9:7700";

static int agent_to_beans(pxy_agent_t *a, rec_msg_t* msg, int msp_unreg)
{
        UNUSED(a);
        char url[32] = {0};
        char uid[15] = {0};
        char sid[15] = {0};
        //sprintf(uid, "%d", a->user_id);
        sprintf(uid, "%d", msg->userid);
        sprintf(sid, "%d", a->sid);

        int r = route_get_app_url(msg->cmd, 1, uid, sid, a->ct, a->cv, url);
        if (r <= 0) {
                I("fd %d, uid %s, epid %s, cmd %d, ct %s, cv %s, get url is null", 
                                a->fd, uid, a->epid, msg->cmd, a->ct, a->cv);
                return -1;
        }

        msg->uri = url;
        //      msg->uri = __url;

        if (mrpc_us_send(msg) < 0) {
                W("fd %d, uid %s, epid %s, cmd %d, url %s mrpc send error", 
                                a->fd, uid, a->epid, msg->cmd, url);
                return -1;
        }

        if (msg->user_context == NULL || msg->user_context_len == 0) {
                W("fd %d, uid %s, epid %s, cmd %d userctx is null", 
                                a->fd, uid, a->epid, msg->cmd);
        }

        I("fd %d, uid %d, epid %s, cmd %d, auid %d, sid %d, seq %u, ct %s, "
                        "cv %s, url=%s sent to backend", a->fd, msg->userid, a->epid, 
                        msg->cmd, a->user_id, a->sid, msg->seq, a->ct, a->cv, url);

        return 0;
}

static int process_client_req(pxy_agent_t *agent)
{
        rec_msg_t msg;
        int r;
        while ((r = parse_client_data(agent, &msg)) > 0) {
                if (agent->user_id > 0) {
                        if (msg.userid != agent->user_id) {
                                msg.body = AUTHERROR;
                                msg.body_len = sizeof(AUTHERROR);
                                msg.format = 128;
                                _send_to_client(&msg, agent);
                                W("userid %d not match, fd %d, uid %d, epid %s," 
                                                " cmd %d", msg.userid, agent->fd, 
                                                agent->user_id, agent->epid, msg.cmd);
                                return -1;
                        }
                }

                //we need to refresh the seq to make sure seq conflict
                //              if (msg.format < 128 && agent->bn_seq <= msg.seq) {
                //                      agent->bn_seq = msg.seq + 1;
                //              }

                struct hashtable *table = agent->svr_req_buf;
                mrpc_req_buf_t *svr_req_data = NULL;

                if (msg.format == -128) {
                        int key = msg.seq;
                        svr_req_data = hashtable_search(table, &key);
                        I("search server request by fd %d, uid %d, epid %s, "
                                        "cmd %d, seq=%d, data=%p, table=%p", agent->fd, 
                                        agent->user_id, agent->epid, msg.cmd, msg.seq, 
                                        svr_req_data, table);

                        if (svr_req_data != NULL) {
                                agent_svr_response(agent, &msg, table, svr_req_data);
                        } 

                        goto finish;
                } else if (agent_to_beans(agent, &msg, 0) < 0) {
                        char time[32];
                        char marker[32];
                        char loggername[64];
                        char message[128];
                        db_gettimestr(time, sizeof(time));
                        sprintf(marker, "%d", msg.userid);
                        sprintf(loggername, "%s:%s:%d", __FILE__, __FUNCTION__, __LINE__);
                        sprintf(message, "bad gateway. cmd:%d.", msg.cmd);
                        db_insert_log(80000, 
                                        0,
                                        getpid(),
                                        time,
                                        loggername,
                                        message,
                                        "",
                                        marker,
                                        "",
                                        "mspc",
                                        setting.ip);

                        if (msg.body_len > 0) {
                                free(msg.body);
                        }

                        msg.body = BADGATEWAY;
                        msg.body_len = sizeof(BADGATEWAY);
                        msg.format = 128; //response

                        if (_send_to_client(&msg, agent) < 0) {
                                E("send BADGATEWAY to client failed fd %d, "
                                                "uid %d, epid %s, cmd %d", agent->fd, 
                                                agent->user_id, agent->epid, msg.cmd);
                                return -1;
                        }

                        E("sent cmd BADGATEWAY fd %d, uid %d, epid %s, cmd %d",
                                        agent->fd, agent->user_id, agent->epid, msg.cmd);

                        continue;
                }

                if (msg.cmd == CMD_USER_UNREG) {
                        agent->isunreg = 1;
                }
finish:

                if (msg.body_len > 0) {
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

        I("agent close fd %d, uid %d, epid %s", a->fd, a->user_id, a->epid);
        free(a->epid);
        a->epid = NULL;
        if (a->user_ctx_len > 0) {
                a->user_ctx_len = 0;
                free(a->user_ctx);
        }
        a->user_ctx = NULL;

        if (a->ct != NULL) free(a->ct);
        if (a->cv != NULL) free(a->cv);

        mrpc_buf_free(a->recv_buf);
        mrpc_buf_free(a->send_buf);

        ev_time_item_ctl(worker->ev, EV_CTL_DEL, a->timer);
        free(a->timer);

        if (a->fd > 0) {
                if (ev_del_file_item(worker->ev, a->fd) < 0) {
                        I("del file item err, errno is %d", errno);
                }

                D("close the socket");
                close(a->fd);
        }

        hashtable_destroy(a->svr_req_buf, -1);
        free(a);
}

#define AGENT_TIMER_INTERVAL 30
void close_idle_agent(ev_t* ev, ev_time_item_t* ti) {
        pxy_agent_t* agent = ti->data;
        time_t now = time(NULL);
        D("check_client_alive_time:%d", setting.check_client_alive_time);

        if (now - agent->last_active > setting.check_client_alive_time) {
                W("close expired idle client! fd %d, uid %d, epid %s", agent->fd,
                                agent->user_id, agent->epid);
                goto failed;
        } else {
                ev_time_item_t *item = ev_time_item_new(worker->ev, agent,
                                agent_timer_handler, 
                                time(NULL) + AGENT_TIMER_INTERVAL);
                if (!item) {
                        W("cannot malloc timer item");
                        goto failed;
                }

                free(ti);
                agent->timer = item;
                if (ev_time_item_ctl(worker->ev, EV_CTL_ADD, item) < 0) {
                        W("cannot add timer fd %d, uid %d, epid %s", agent->fd,
                                        agent->user_id, agent->epid);
                        goto failed;
                }
        }
        return;

failed:
        worker_remove_agent(agent);
        pxy_agent_close(agent);

}
void check_svr_req_buf(ev_t *ev, ev_time_item_t* ti) {
        pxy_agent_t *agent = ti->data;
        time_t now = time(NULL);

        // TODO iterator hashtable check if need to remove timeout request buf
        // item 
        int keycount = hashtable_count(agent->svr_req_buf); 
        if (keycount <= 0) return;

        struct hashtable_itr *itr = hashtable_iterator(agent->svr_req_buf);
        if (itr == NULL) return;

        int *keys = calloc(keycount + 1, sizeof(*keys));
        int *p = keys;
        do {
                mrpc_req_buf_t *req = hashtable_iterator_value(itr);

                if (req != NULL && now - req->time > 0) {
                        *p = req->sequence; p++;
                }
        } while (hashtable_iterator_advance(itr));
        if (itr != NULL) free(itr);

        p = keys;
        while (*p != 0) {
                mrpc_req_buf_t *req = hashtable_remove(agent->svr_req_buf, p);
                if (NULL == req) {
                        E("remove svr req buf failed! key=%d fd %d uid %d "
                                        "epid %s", *p, agent->fd, agent->user_id, agent->epid);
                        p++; 
                        continue;
                } else if (req != NULL && req->c->conn_status == MRPC_CONN_CONNECTED) {
                        I("remove svr req buf ok, key=%d fd %d uid %d epid %s",
                                        *p, agent->fd, agent->user_id,
                                        agent->epid);
                        rec_msg_t m;
                        m.userid = req->userid;
                        m.seq = req->sequence;
                        m.cmd = req->cmd;
                        m.compress = req->compress;
                        m.logic_pool_id = agent->logic_pool_id;
                        m.epid = agent->epid;

                        if (agent->user_ctx != NULL && agent->user_ctx_len > 0) {
                                m.user_context = agent->user_ctx;
                                m.user_context_len = agent->user_ctx_len;
                        }

                        m.body = OVERTIME;
                        m.body_len = sizeof(OVERTIME);
                        m.format = 128; 

                        W("client response timeout code 504 cmd %d userid %d epid %s",
                                        m.cmd, m.userid, m.epid);
                        int ret = send_svr_response(req, &m);
                        if (ret < 0) {
                                W("send response code 504 failed: cmd %d userid %d epid %s",
                                                m.cmd, m.userid, m.epid);
                        }
                } else if (req != NULL && req->c->conn_status != MRPC_CONN_CONNECTED) {
                        W("send response code 504 failed, backend conn broken! cmd %d "
                                        "userid %d epid %s conn_status %d %p", req->cmd, req->userid, 
                                        agent->epid, req->c->conn_status, req->c);
                }
                req->c->refs -= 1;

                free(req);
                p++;
        }

        free(keys);
}

void agent_timer_handler(ev_t* ev, ev_time_item_t* ti)
{
        check_svr_req_buf(ev, ti);
        close_idle_agent(ev, ti);
        mrpc_check_stash_conns();
}

static unsigned int svr_req_hash(void* id) 
{
        unsigned int ret = murmur_hash2(id, sizeof(unsigned int));
        //I("hashvalue: %u", ret);
        return ret;
}

static int svr_req_cmp_f(void* d1, void*d2) 
{
        //I("k1=%d, k2=%d", *((int*)d1), *((int*)d2));
        return *((int*)d1) - *((int*)d2) == 0 ? 1 : 0;
}

pxy_agent_t * pxy_agent_new(int fd, int userid, char *client_ip)
{
        pxy_agent_t *agent = calloc(1, sizeof(*agent));
        if (!agent) {
                E("no mempry for agent"); 
                goto failed;
        }

        agent->send_buf = mrpc_buf_new(MSP_BUF_LEN);
        if (!agent->send_buf) {
                E("no mem for agent send_buf");
                goto failed;
        }

        agent->recv_buf = mrpc_buf_new(MSP_BUF_LEN);
        if (!agent->recv_buf) {
                mrpc_buf_free(agent->send_buf);
                E("no mem for agent recv_buf");
                free(agent);
                goto failed;
        }

        agent->svr_req_buf = create_hashtable(8, svr_req_hash, svr_req_cmp_f);

        agent->timer = ev_time_item_new(worker->ev, agent,
                        agent_timer_handler, 
                        time(NULL) + 10);
        if (!agent->timer) {
                W("no mem for agent timer");
                mrpc_buf_free(agent->send_buf);
                mrpc_buf_free(agent->recv_buf);
                free(agent);
                goto failed;
        }       
        if (ev_time_item_ctl(worker->ev, EV_CTL_ADD, agent->timer) < 0) {
                W("add timer error");
                mrpc_buf_free(agent->send_buf);
                mrpc_buf_free(agent->recv_buf);
                free(agent->timer);
                free(agent);
                goto failed;
        }

        agent->fd = fd;
        agent->user_id = userid;
        agent->epid = NULL;
        agent->ct = NULL;
        agent->cv = NULL;

        char t_userctx[1024] = {0};
        struct pbc_slice s_userctx = {t_userctx, sizeof(t_userctx)};

        user_context userctx;
        memset(&userctx, 0, sizeof(userctx));
        userctx.client_ip.len  = strlen(client_ip);
        userctx.client_ip.buffer = strdup(client_ip);

        int r1 = pbc_pattern_pack(rpc_pat_user_context, &userctx, &s_userctx);
        if (r1 < 0) {
                E("pack client_ip args error!");
                goto failed;
        }

        agent->user_ctx = strdup(t_userctx);
        agent->user_ctx_len = sizeof(t_userctx) - r1;
        agent->isreg = 0;
        agent->upflow = 0;
        agent->downflow = 0;
        agent->isunreg = 0;
        return agent;
failed:
        return NULL;
}

int agent_send_netstat(pxy_agent_t *agent)
{
        if (agent->user_id <= 0) {
                return 0;
        }

        I("fd %d, epid %s, uid %d, cmd 104 send netstat",agent->fd, agent->epid,
                        agent->user_id);
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
        if (r < 0) {
                W("pack args error fd %d, uid %d, epid %s, cmd 104", agent->fd, 
                                agent->user_id, agent->epid);
                return -1;
        }

        msg.body_len = 128 -r;
        msg.body = tmp;
        agent_to_beans(agent, &msg, 0);
        return 0;
}

int agent_send_offstate(pxy_agent_t *agent)
{
        if (agent->user_id <= 0 || agent->isunreg == 1 ) {
                return 0;
        }

        rec_msg_t msg = {0};

        I("fd %d, uid %d, epid %s, cmd 103 send offstate", agent->fd, 
                        agent->user_id, agent->epid);
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
void agent_recv_client(ev_t *ev,ev_file_item_t *fi)
{
        UNUSED(ev);
        pxy_agent_t *agent = fi->data;

        int n = mrpc_recv2(agent->recv_buf,agent->fd);
        if (n == 0) {
                I("recv client == 0 fd %d, uid %d, epid %s", agent->fd, 
                                agent->user_id, agent->epid);
                goto failed;
        }
        agent->upflow += n;

        if (process_client_req(agent) < 0) {
                I("process client request failed fd %d, uid %d, epid %s", 
                                agent->fd, agent->user_id, agent->epid);
                goto failed;
        }
        if (agent->recv_buf->offset == agent->recv_buf->size) {
                mrpc_buf_reset(agent->recv_buf);
        }
        time_t now = time(NULL); 
        agent->last_active = now;
        return;

failed:
        worker_remove_agent(agent);
        pxy_agent_close(agent);
        return;
}

void agent_send_client(ev_t *ev, ev_file_item_t *fi)
{
        UNUSED(ev);
        pxy_agent_t *a = fi->data;
        int r = mrpc_send2(a->send_buf, a->fd);

        if (r < 0) {
                I("send to client failed, prepare close, fd %d, uid %d, epid %s", 
                                a->fd, a->user_id, a->epid);
                worker_remove_agent(a);
                pxy_agent_close(a);
                return;
        }

        I("succ send %d bytes to fd %d, uid %d, epid %s", r, a->fd, a->user_id,
                        a->epid);
        return;
}

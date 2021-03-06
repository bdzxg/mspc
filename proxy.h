#ifndef _PROXY_H_
#define _PROXY_H_

#define MAX_EVENTS 1000
#define BUFFER_SIZE 500
#define pxy_memzero(buf, n)       (void) memset(buf, 0, n)
#define UNUSED(x) (void)(x)

#include "sysinc.h"
#include "rbtree.h"
#include "murmur_hash.h"
#include "ev.h"
#include "include/pbc.h"
#include "list.h"
#include "config.h"
#include "proto.h"
#include "mrpc.h"
#include "agent.h"
#include "map.h"
#include "log.h"
#include "upstream.h"
#include "tool.h"
#include "settings.h"
#include "route.h"
#include "msp_pb.h"
#include "mrpc_pb.h"
#include "hashtable.h"
//#include "hashtable_itr.h"
#include "include/pbc.h"
#include "mrpc_msp.h"

typedef struct {
	char *data;
	size_t len;
}slice;

typedef struct pxy_worker_s{
    int bfd;
    ev_t* ev;
    int connection_n;
    pid_t pid;
    struct sockaddr_in *baddr;
    int socket_pair[2];
	struct rb_root root;
	FILE* fid;
	struct rb_root r3_root;
}pxy_worker_t;

typedef struct pxy_master_s{
    int listen_fd;
    struct sockaddr addr;
}pxy_master_t;

typedef struct pxy_command_s{
#define PXY_CMD_QUIT 1
    pid_t pid;
    int cmd;
    int fd;
}pxy_command_t;

#define ev_file_free free

int worker_init();
int worker_start();
int worker_close();
void worker_accept(ev_t*,ev_file_item_t*);
int worker_recv_client(ev_t*,ev_file_item_t*);
void worker_recv_cmd(ev_t*,ev_file_item_t*);
char* get_send_data(rec_msg_t* t, int* length);
void *rpc_server_thread(void *arg);
void* recycle_connection_reg3_thread(void* args);

static inline 
int setnonblocking(int sock)
{
    int opts;
    opts = fcntl(sock,F_GETFL);
    if(opts<0) return -1;

    opts = opts | O_NONBLOCK;
    if(fcntl(sock,F_SETFL,opts) < 0)
	return -1;

    return 0;
}

static inline 
void iov_init(struct iovec *iov,void *base,int len)
{
    iov->iov_base = base;
    iov->iov_len = len;
}

#endif

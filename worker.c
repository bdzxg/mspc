#include "proxy.h"
#include "route.h"
#include "map.h"
#include "upstream.h"
#include "agent.h"

extern pxy_master_t *master;
extern pxy_worker_t *worker;
extern pxy_settings setting;
upstream_map_t *upstream_root;

int worker_init();
int worker_start();
int worker_close();
void worker_recv_cmd(ev_t*,ev_file_item_t*);
void worker_ev_after(ev_t *);

int worker_init()
{
	worker = calloc(1, sizeof(*worker));

	if(worker) {
		worker->ev = ev_create2(NULL, 1024 * 100);
		if(!worker->ev){
			D("create ev error"); 
			return -1;
		}
		worker->ev->after = worker_ev_after;

		upstream_root = (upstream_map_t*) calloc(1, sizeof(*upstream_root));
		if(!upstream_root) {
			E("cannnot malloc for upstream_root");
			return -1;
		}
		upstream_root->root = RB_ROOT;

		worker->root = RB_ROOT;
		if(mrpc_init() < 0) {
			E("mrpc init error");
			return -1;
		}
		
		if(msp_args_init() < 0) {
			E("rpc args init error");
			return -1;
		}

		return 0;
	}

	return -1;
}

int worker_start()
{
	route_init(setting.zk_url);
	int fd = master->listen_fd;
	int r = ev_add_file_item(worker->ev, 
				 fd, 
				 EV_READABLE, 
				 NULL, 
				 worker_accept, NULL);
	if(r < 0) {
		E("add listen fd ev error %s", strerror(errno));
		goto start_failed;
	}

	if(mrpc_start() < 0) {
		E("mrpc start error");
		goto start_failed;
	}

	ev_main(worker->ev);
	return 1;
 start_failed:
	return -1;
} 

int
worker_close()
{
	route_close();
	pxy_agent_t *item;
	struct rb_node *rn;
	map_walk(&worker->root,rn) {
		item = rb_entry(rn, struct pxy_agent_s, rbnode);
		D("queue name is %s:%p\n",item->epid,item);
		pxy_agent_close(item);
	}
	upstream_t *us;
	struct rb_node *r;
	map_walk(&upstream_root->root, r)
	{
		us = rb_entry(r, struct upstream_s, rbnode);
		release_upstream(us);
	}

	worker->ev->stop = 1;
	free(worker->ev->api_data);
	close(master->listen_fd);
	return 0;
} 
 

void worker_accept(ev_t *ev, ev_file_item_t *ffi)
{
	UNUSED(ev);

	int f,err;
	pxy_agent_t *agent;

	socklen_t sin_size = sizeof(master->addr);
	f = accept(ffi->fd,&(master->addr),&sin_size);
	
        if(f>0){
        	int k = 1;
        	if(setsockopt(f, SOL_SOCKET, SO_KEEPALIVE, (void *)&k, sizeof(k)) < 0) {
        		E("set keep alive failed, reason %s ",
        		  strerror(errno));
        		close(f);
        		return;
        	}
        	D("fd#%d accepted",f);

		/* FIXME:maybe we should try best to accept and 
		 * delay add events */
		err = setnonblocking(f);
		if(err < 0){
			W("set nonblocking error"); return;
		}

		agent = pxy_agent_new(f, 0);
		if(!agent){
			W("create new agent error"); return;
		}
		
		int r = ev_add_file_item(worker->ev, 
					 f, 
					 EV_READABLE | EV_WRITABLE | EPOLLET,
					 agent, 
					 agent_recv_client, 
					 agent_send_client);
		
		if(r < 0) {
			W("add file item failed");
			return;
		}
                I("comming new agent: fd: %d", f);
	}
	else {
		W("accept %s", strerror(errno));
	}
}

void 
worker_recv_cmd(ev_t *ev,ev_file_item_t *fi)
{
	UNUSED(ev);
	UNUSED(fi);
	/* only for quit now */
	worker_close();
}

void worker_ev_after(ev_t *ev)
{
	UNUSED(ev);
	mrpc_ev_after();
}


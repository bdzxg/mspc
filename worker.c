
#include "proxy.h"
#include "route.h"
#include "map.h"
#include "upstream.h"
#include "agent.h"

extern pxy_master_t *master;
extern pxy_worker_t *worker;
freeq_t *request_q;
upstream_map_t *upstream_root;

int worker_init();
int worker_start();
int worker_close();
void worker_accept(ev_t*,ev_file_item_t*);
void worker_recv_client(ev_t*,ev_file_item_t*);
void worker_recv_cmd(ev_t*,ev_file_item_t*);
void process_request_in_queue(struct ev_s *ev);

int worker_init()
{
	worker = (pxy_worker_t*)pxy_calloc(sizeof(*worker));

	if(worker) {
		worker->ev = ev_create();
		if(!worker->ev){
			D("create ev error"); 
			return -1;
		}
		worker->ev->after = process_request_in_queue;

		worker->agent_pool = mp_create(sizeof(pxy_agent_t),0,"AgentPool");
		if(!worker->agent_pool){
			D("create agent_pool error");
			return -1;
		}

		worker->buf_data_pool = mp_create(BUFFER_SIZE,0,"BufDataPool");
		if(!worker->buf_data_pool){
			D("create buf_data_pool error");
			return -1;
		}

		worker->buf_pool = mp_create(sizeof(buffer_t),0,"BufPool");
		if(!worker->buf_pool) {
			D("create buf_pool error");
			return -1;
		}

		upstream_root = (upstream_map_t*) pxy_calloc(sizeof(*upstream_root));
		if(!upstream_root) {
			E("cannnot malloc for upstream_root");
			return -1;
		}
		upstream_root->root = RB_ROOT;

		worker->root = RB_ROOT;
		pthread_mutex_init(&worker->mutex, NULL);


		request_q = create_free_q();
		if(!request_q) {
			E("cannot create request q");
			return -1;
		}

		return 0;
	}

	return -1;
}

int worker_start()
{
	//TODO: Hard coding 
	//char *zk_url= "192.168.110.231:8998"; //sitec
	//char *zk_url = "192.168.199.8:2181";
	
	char *zk_url= "192.168.110.125:8998";// sitea
	route_init(zk_url);

	ev_file_item_t* fi ;
	int fd = master->listen_fd;

	fi = ev_file_item_new(fd, worker, worker_accept, NULL, EV_READABLE);
	if(!fi){
		D("create ev for listen fd error");
		goto start_failed;
	}
	ev_add_file_item(worker->ev,fi);

	/* TODO: Backend todo
	   worker->bfd = socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);
	   if(!worker->bfd){
	   goto start_failed;
	   }

	   if(!connect(worker->bfd,(struct sockaddr*)worker->baddr,
	   sizeof(*(worker->baddr)))){
	   goto start_failed;
	   }  */

	ev_main(worker->ev);
	free(fi);
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
 

void
worker_accept(ev_t *ev, ev_file_item_t *ffi)
{
	UNUSED(ev);

	int f,err;
	pxy_agent_t *agent;
	ev_file_item_t *fi;

	//for(i=0;i<100;i++){
	/*try to accept 100 times*/
	socklen_t sin_size = sizeof(master->addr);
	f = accept(ffi->fd,&(master->addr),&sin_size);
	D("fd#%d accepted",f);

	if(f>0){

		/* FIXME:maybe we should try best to accept and 
		 * delay add events */
		err = setnonblocking(f);
		if(err < 0){
			D("set nonblocking error"); return;
		}

		agent = pxy_agent_new(worker->agent_pool,f,0);
		if(!agent){
			D("create new agent error"); return;
		}

		fi = ev_file_item_new(f,
				      agent,
				      agent_recv_client,
				      NULL,
				      EV_READABLE | EPOLLET);
		if(!fi){
			D("create file item error");
		}
		ev_add_file_item(worker->ev,fi);

		//map_insert(&worker->root,agent);
		/*TODO: check the result*/
	}	
	else {
		perror("accept");
	}
	//}
}

void 
worker_recv_cmd(ev_t *ev,ev_file_item_t *fi)
{
	UNUSED(ev);
	UNUSED(fi);
	/* only for quit now */
	worker_close();
}

void process_request_in_queue(struct ev_s *ev)
{
	UNUSED(ev);
	rec_msg_t *msg;
	void *req;
	while((req = freeq_pop(request_q)) != NULL) {
		msg = req;

		if(msg->cmd == 105)
			// TODO add tcp flow
			return;
		// can not be 102
		int len;
		char* data = get_send_data(msg, &len);
		//TODO: WHY>/....
		//send_to_client(a, data, &len);
		free(data);
		free(msg->body);
		free(msg);
	}
}


/*
 * Copyright liulijin<llj098@gmail.com>
 */ 

#include "proxy.h"
#include "agent.h"
#include "settings.h"

pxy_master_t* master;
pxy_worker_t* worker;
FILE* log_file;
int log_level = 0;
extern pxy_settings setting;
//pxy_settings setting = {
//        LOG_LEVEL_DEBUG,
//	"mspc.txt",
//        9014,
//        9015,
//	"0.0.0.0",
//	LOG_LEVEL_DEBUG,
//	"route_log.txt",
//	"192.168.110.125:8998"
//};

int
pxy_setting_init(char *conf_file)
{ 
	int r;
	config_item *item = calloc(1, sizeof(*item));
	if (!item) {
		E("malloc config_item error");
		return -1;
	}
	FILE *file = fopen(conf_file, "r");
	if (file == NULL) {
		file = stdin;
	}

	while((r = load_config(file,item)) != 0) {
		if (r == -1) {
			continue;
		}

		if (strcmp(item->name,"log_level") == 0) { 
			setting.log_level = atoi(item->value);
                        log_level = setting.log_level;
                }

		if (strcmp(item->name,"log_file") == 0) {
			int len =  strlen(item->value) + 1;
			strncpy(setting.log_file, item->value, len);
                        if (strcmp(setting.log_file, "stdout") == 0 ||
                            strcmp(setting.log_file, "stderr") == 0) {
                                log_file = stdout;                        
                        } else {
                                log_file = fopen(setting.log_file, "a");
                                 
                                if (NULL == log_file) {
                                        log_file = stdout;
                                        E("LOG FILE open failed!");
					goto ERROR;
                                }

                                fclose(log_file);
                        }
		}

		if (strcmp(item->name,"client_port") == 0) 
			setting.client_port= atoi(item->value);		
                if (strcmp(item->name,"backend_port") == 0) 
			setting.backend_port = atoi(item->value);
		if (strcmp(item->name,"ip") == 0) 
			strcpy(setting.ip, item->value);
		if (strcmp(item->name,"route_log_level") == 0) {
			setting.route_log_level = atoi(item->value);
			route_set_loglevel(setting.route_log_level);
		}
		if (strcmp(item->name,"route_log_file") == 0) {
			strcpy(setting.route_log_file, item->value);
		}
                if (strcmp(item->name, "route_server_port") == 0) {
                        setting.route_server_port = atoi(item->value);
                }
                if (strcmp(item->name, "flush_log") == 0) {
                        setting.is_flush_log = atoi(item->value);
                }
		if (strcmp(item->name, "zk_url") == 0)
			strcpy(setting.zk_url, item->value);
		memset(item, 0, sizeof(*item));
	}
	
	E("setting inited: setting.log_level %d, log_file %s, client_port %d,"
                        "backend_port %d, ip %s, route_log_level %d, "
                        "route_log_file %s, route_server_port %d, zk_url %s",
	  setting.log_level,
	  setting.log_file,
	  setting.client_port,
	  setting.backend_port,
	  setting.ip,
	  setting.route_log_level,
	  setting.route_log_file,
          setting.route_server_port,
	  setting.zk_url);
	
	return 0;
ERROR:
	if (file != stdin) {
		fclose(file);
	}
	
	if (item) {
		free(item);
	}

	return -1;
}

void
pxy_master_close()
{
	if (master->listen_fd > 0) {
		D("close the listen fd");
		close(master->listen_fd);
	}
}

int
pxy_send_command(pxy_worker_t *w,int cmd,int fd)
{
	struct msghdr m;
	struct iovec iov[1];
	pxy_command_t *c = malloc(sizeof(*c));

	if (!c) {
		D("no memory");
		return -1;
	}

	c->cmd = cmd;
	c->fd = fd;
	c->pid = w->pid;

	iov[0].iov_base = c;
	iov[0].iov_len = sizeof(*c);

	m.msg_name = NULL;
	m.msg_namelen = 0;
	m.msg_control = NULL;
	m.msg_controllen = 0;
  
	m.msg_iov = iov;
	m.msg_iovlen = 1;

	int a;
	if ((a=sendmsg(w->socket_pair[0],&m,0)) < 0) {
		free(c);
		D("send failed%d",a);
		return  -1 ;
	}

	free(c);
	D("master sent cmd:%d to pid:%d fd:%d", cmd, w->pid, fd);
	return 0;
}

int 
pxy_start_listen()
{
	struct sockaddr_in addr1;

	master->listen_fd = socket(AF_INET,SOCK_STREAM,0);
	if (master->listen_fd < 0) {
		D("create listen fd error");
		return -1;
	}

	int reuse = 1;
	setsockopt(master->listen_fd,
		   SOL_SOCKET,
		   SO_REUSEADDR,
		   &reuse,
		   sizeof(int));
	
	if (setnonblocking(master->listen_fd) < 0) {
		D("set nonblocling error");
		return -1;
	}

	addr1.sin_family = AF_INET;
	addr1.sin_port = htons(setting.client_port);
	inet_aton(setting.ip, &addr1.sin_addr);
	
	if (bind(master->listen_fd, (struct sockaddr*)&addr1,
		sizeof(addr1)) < 0) {
		E("bind address %s:%d error", setting.ip,
		  setting.client_port);
		return -1;
	}

	if (listen(master->listen_fd,1000) < 0) {
		E("listen error");
		return -1;
	}
    
	return 0;
}

int 
pxy_init_master()
{
	master = (pxy_master_t*)malloc(sizeof(*master));
	if (!master) {
		E("no memory for master");
		return -1;
	}


	return pxy_start_listen();
}

void set2buf(char** buf, char* source, int len)
{
	int j;
	for(j = 0; j < len; j++) {
		**buf = *(source+j);	
		(*buf)++;
	}
}

char* get_send_data(rec_msg_t* t, int* length)
{
	int padding = 0;
	int offset = 24;
	*length = 25 + t->body_len;
	char* ret = calloc(sizeof(char), *length);
	if (ret == NULL)
		return ret;

	t->version = 0;//msp use 0.
	char* rval = ret;
	set2buf(&rval, (char*)&padding, 1);
	// length
	set2buf(&rval, (char*)length, 2);	
	// version
	set2buf(&rval, (char*)&t->version, 1);
	// userid
	set2buf(&rval, (char*)&t->userid, 4);
	// cmd
	set2buf(&rval, (char*)&t->cmd, 2);
	// seq
	set2buf(&rval, (char*)&t->seq, 2);
	// offset
	set2buf(&rval, (char*)&offset, 1);
	// format
	set2buf(&rval, (char*)&t->format, 1);
	// zipflag compress
	set2buf(&rval, (char*)&t->compress, 1);
	// clienttype
	set2buf(&rval, (char*)&t->client_type, 2);
	// clientversion
	set2buf(&rval, (char*)&t->client_version, 2);
	// 0
	set2buf(&rval, (char*)&padding, 1);
	// option use 0 template
	set2buf(&rval, (char*)&padding, 4);
	/*
	  int i;
	  if (t->option_len != 0)
	  for(i = 0; i < t->option_len; i++)
	  *(rval++) = *(t->option++);
	  else rval += 4;*/

	// body
	
	char tmp[102400] = {0};
	to_hex(t->body, t->body_len, tmp);

	D("body is %s", tmp);

	memcpy(ret + 24, t->body, t->body_len);
	return ret;
}

int main(int argc, char** argv)
{
	log_file = stdout;
        init_log();
	D("process start");
	char ch[80];

        D("Start init settings!");
        char conf_file[256];

        if (argc > 1) {
                strcpy(conf_file, argv[1]);
        }
        else {
                strcpy(conf_file, "mspc.conf");
        }

        if (pxy_setting_init(conf_file) != 0) {
                D("settings initialize failed!\n");
                return -1;
        }

	if (pxy_init_master() < 0) {
		E("master initialize failed");
		return -1;
	}
	D("master initialized");


	/* TODO: Maybe we need remove the master-worker mode,
	   seems useless
	 */

	if (worker_init()<0) {
		E("worker #%d initialized failed" , getpid());
		return -1;
	}
	D("worker inited");

	if (worker_start() < 0) {
		E("worker #%d started failed", getpid()); 
		return -1;
	}
	D("worker started");


	while(scanf("%s",ch) >= 0 && strcmp(ch,"quit") !=0) { 
	}

	sleep(5);
	D("pxy_master_close");
	pxy_master_close();
	return 1;
}

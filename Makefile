CC = 	gcc
CFLAGS = -pipe -Wall -I -g -O2 -ggdb
LINK =	$(CC)

LIB_OBJS = \
	./rbtree.o \
	./map.o \
	./ev.o \
	./settings.o \
	./log.o \
	./tool.o
#./hashtable.o \
	./hashtable_itr.o \

MAP_OBJS = \
	./rbtree.o \
	./map.o 

FUNC_OBJS = \
	./func_test.o 

PXY_OBJS = \
	./worker.o \
	./proxy.o \
	./agent.o \
	./upstream.o \
	./msp_pb.o \
	./config.o

MRPC_OBJS = \
	./mrpc.o \
	./mrpc_cli.o \
	./mrpc_svr.o \
	./mrpc_common.o \
	./mrpc_pb.o \
	./mrpc_msp.o

PXY_TEST = \
	./pxy_test.o $(LIB_OBJS) \

HT_TEST = \
	./hashtable_test.o \

MAP_TEST = \
	./map_test.o

EV_TEST = \
	./ev_test.o

HASH_TEST = \
	./test_hash.o

FUNC_TEST = \
	./func_test.o

TEST = \
	$(PXY_TEST) \
	$(HT_TEST)  \

OUTPUT = mspc
all : apl 
apl:  $(PXY_OBJS) $(LIB_OBJS) $(MRPC_OBJS)
	$(LINK) $(PXY_OBJS) $(LIB_OBJS) $(MRPC_OBJS) -I./include -o $(OUTPUT) \
		-Llib -lmclibc -lzookeeper_mt -lprotobuf-c -lm -lroute  -lpthread \
		-lpbc 

ev_test : $(LIB_OBJS) $(EV_TEST)
	$(LINK) $(LIB_OBJS) $(EV_TEST) -o ev_test -lm -lmclibc

hash_test :  $(HASH_TEST)
	$(LINK) $(HASH_TEST) -o hash_test  -Llib -lmclibc -lm
	 
clean:
	rm -f *.o
	rm -f $(PXY_OBJS)
	rm -f $(LIB_OBJS)
	rm -f $(OUTPUT)
	rm -f $(TEST)
	rm -f ev_test
	rm -f $(MAP_TEST) 
	rm -f $(FUNC_TEST)
	rm -f $(MRPC_OBJS)

map_test: $(MAP_OBJS) $(MAP_TEST)
	$(LINK) $(MAP_OBJS) $(MAP_TEST) -o $@


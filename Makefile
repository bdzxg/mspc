CC = 	gcc
CFLAGS = -pipe -Wall -I -g -O0 -ggdb
LINK =	$(CC)

LIB_OBJS = \
	./rbtree.o \
	./map.o \
	./ev.o \
	./mempool.o \
	./tool.o

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
	./mrpc_pb.o

PXY_TEST = \
	./pxy_test.o \

HT_TEST = \
	./hashtable_test.o \

MAP_TEST = \
	./map_test.o

FUNC_TEST = \
	./func_test.o

TEST = \
	$(PXY_TEST) \
	$(HT_TEST)  \

OUTPUT = mspc

apl:  $(LIB_OBJS) $(PXY_OBJS) $(MRPC_OBJS)
	$(LINK)	$(LIB_OBJS) $(PXY_OBJS) $(MRPC_OBJS) -I./include -o $(OUTPUT) -Llib -lzookeeper_mt -lprotobuf-c -lm -lroute -lev -lpthread -lpbc

clean:
	rm -f $(PXY_OBJS)
	rm -f $(LIB_OBJS)
	rm -f $(OUTPUT)
	rm -f $(TEST)
	rm -f $(MAP_TEST) 
	rm -f $(FUNC_TEST)
	rm -f $(MRPC_OBJS)

map_test: $(MAP_OBJS) $(MAP_TEST)
	$(LINK) $(MAP_OBJS) $(MAP_TEST) -o $@


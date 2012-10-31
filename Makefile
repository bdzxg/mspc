CC = 	gcc
CFLAGS = -pipe -O0 -W -Wall -I -g
LINK =	$(CC)

LIB_OBJS = \
	./rbtree.o \
	./map.o \
	./ev.o \
	./mempool.o \
	./hashtable.o \
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
	./mrpc.o

PXY_TEST = \
	./pxy_test.o \

HT_TEST = \
	./hashtable_test.o \

MAP_TEST = \
	./map_test.o

FUNC_TEST = \
	./func_test.o

FREEQ_OBJS = \
	./freeq.o \
	./freeq_test.o

TEST = \
	$(PXY_TEST) \
	$(HT_TEST)  \

OUTPUT = mspc

apl:  $(LIB_OBJS) $(PXY_OBJS) 
	$(LINK)	$(LIB_OBJS) $(PXY_OBJS) -I./include -o $(OUTPUT) -Llib -lzookeeper_mt -lprotobuf-c -lm -lroute_mt -lev -lpthread -lpbc

clean:
	rm -f $(PXY_OBJS)
	rm -f $(LIB_OBJS)
	rm -f $(OUTPUT)
	rm -f $(TEST)
	rm -f $(MAP_TEST) 
	rm -f $(FUNC_TEST)

ht_test: $(LIB_OBJS) $(HT_TEST)
	$(LINK) $(LIB_OBJS) $(TEST) -o $@

map_test: $(MAP_OBJS) $(MAP_TEST)
	$(LINK) $(MAP_OBJS) $(MAP_TEST) -o $@

freeq_test: $(FREEQ_OBJS) 
	$(LINK) $(FREEQ_OBJS) -o $@

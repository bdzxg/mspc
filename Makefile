CC = 	gcc
CFLAGS = -pipe  -O0 -Wall -I -g -Werror -ggdb
LINK =	$(CC)

LIB_OBJS = \
	./rbtree.o \
	./map.o \
	./ev.o \
	./mempool.o \
	./hashtable.o \

MAP_OBJS = \
	./rbtree.o \
	./map.o 
FUNC_OBJS = \
	./func_test.o 

PXY_OBJS = \
	./worker.o \
	./proxy.o \
	./agent.o \
	./upstream.o

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

apl:  $(LIB_OBJS) $(PXY_OBJS) 
	$(LINK)	$(LIB_OBJS) $(PXY_OBJS) -I./include -o $(OUTPUT) -Llib -lrpc_c -lzookeeper_mt -lprotobuf-c -lm -lroute_mt -lev -lpthread -lpbc

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


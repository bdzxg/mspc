CC = 	gcc
CFLAGS = -pipe  -O -W -Wall -I -Wno-unused-parameter -g
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

PXY_OBJS = \
	./worker.o \
	./proxy.o \
	./agent.o \

PXY_TEST = \
	./pxy_test.o \

HT_TEST = \
	./hashtable_test.o \

MAP_TEST = \
	./map_test.o

TEST = \
	$(PXY_TEST) \
	$(HT_TEST)  \

OUTPUT = proxy

apl:  $(LIB_OBJS) $(PXY_OBJS) 
	$(LINK)	$(LIB_OBJS) $(PXY_OBJS) -I./include -o $(OUTPUT) -Llib -lrpc_c -lprotobuf-c -lm -lroute_mt -lev -lpthread -lpbc

clean:
	rm -f $(PXY_OBJS)
	rm -f $(LIB_OBJS)
	rm -f $(OUTPUT)
	rm -f $(TEST)

ht_test: $(LIB_OBJS) $(HT_TEST)
	$(LINK) $(LIB_OBJS) $(TEST) -o $@

map_test: $(MAP_OBJS) $(MAP_TEST)
	$(LINK) $(MAP_OBJS) $(MAP_TEST) -o $@


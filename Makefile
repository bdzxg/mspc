CC = 	gcc
CFLAGS = -pipe  -O -W -Wall -I -Wno-unused-parameter -g
LINK =	$(CC)

LIB_OBJS = \
	./ev.o \
	./mempool.o \
	./hashtable.o \
	./rbtree.o\
	./McpAppBeanProto.pb-c.o

PXY_OBJS = \
	./worker.o \
	./proxy.o \
	./agent.o \
	./ClientHelper.o

PXY_TEST = \
	./pxy_test.o \

HT_TEST = \
	./hashtable_test.o \

TEST = \
	$(PXY_TEST) \
	$(HT_TEST)  \

OUTPUT = proxy

all:  $(LIB_OBJS) $(PXY_OBJS) 
	$(LINK)	$(LIB_OBJS) $(PXY_OBJS) -o $(OUTPUT) -Llib -I./include -lrpc_uds -lprotobuf-c -lm -lroute_mt 

clean:
	rm -f $(PXY_OBJS)
	rm -f $(LIB_OBJS)
	rm -f $(OUTPUT)
	rm -f $(TEST)

ht_test: $(LIB_OBJS) $(HT_TEST)
	$(LINK) $(LIB_OBJS) $(TEST) -o $@


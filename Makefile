
CC = 	gcc
CFLAGS = -pipe  -O -W -Wall -Wpointer-arith -Wno-unused-parameter -Wunused-variable -Wunused-value -Werror -g 
LINK =	$(CC)



LIB_OBJS = \
	./mempool.o \
	./hashtable.o

PXY_OBJS = \
	./proxy.o \
	./ev.o 

HT_TEST = \
	./hashtable_test.o \

TEST = \
	$(HT_TEST)

OUTPUT = proxy

all:  $(LIB_OBJS) $(PXY_OBJS) 
	$(LINK) $(LIB_OBJS) $(PXY_OBJS) -o $(OUTPUT)

clean:
	rm -f $(PXY_OBJS)
	rm -f $(LIB_OBJS)
	rm -f $(OUTPUT)
	rm -f $(TEST)

ht_test: $(LIB_OBJS) $(HT_TEST)
	$(LINK) $(LIB_OBJS) $(TEST) -o $@


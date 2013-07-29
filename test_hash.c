#include <stdio.h>
#include <stdlib.h>
#include "hashtable.h"
#include <stdint.h>

static uint32_t
murmur_hash2(u_char *data, size_t len)
{
	uint32_t  h, k;

	h = 0 ^ len;

	while (len >= 4) {
		k  = data[0];
		k |= data[1] << 8;
		k |= data[2] << 16;
		k |= data[3] << 24;

		k *= 0x5bd1e995;
		k ^= k >> 24;
		k *= 0x5bd1e995;

		h *= 0x5bd1e995;
		h ^= k;

		data += 4;
		len -= 4;
	}

	switch (len) {
	case 3:
		h ^= data[2] << 16;
	case 2:
		h ^= data[1] << 8;
	case 1:
		h ^= data[0];
		h *= 0x5bd1e995;
	}

	h ^= h >> 13;
	h *= 0x5bd1e995;
	h ^= h >> 15;

	return h;
}


static unsigned int svr_req_hash(void* id) 
{
	return murmur_hash2(id, sizeof(unsigned int));
}

static int svr_req_cmp_f(void* d1, void*d2) 
{
        printf("k1=%d, k2=%d\n", *((int*)d1), *((int*)d2));
        return *((int*)d1) - *((int*)d2) == 0 ? 1 : 0;
}


int main(int arc, char** argv)
{

        int seq1 = 4;
        int seq2 = 4;
        
        struct hashtable* table =  create_hashtable(8, svr_req_hash, svr_req_cmp_f);
        int r = hashtable_insert(table, &seq1, "test1");
        char * res = hashtable_search(table, &seq2);
        
        printf("r=%d,  res=%s\n", r, res);
        
}

/*
 * Copyright (C) Austin Appleby
 */

#ifndef _MURMUR_HASH_H_
#define _MURMUR_HASH_H_

static inline uint32_t
murmur_hash2(u_char *data, size_t len)
{
	uint32_t  h, k;

	h = 0 ^ len;

	while (len >= 4) {
                fprintf(stderr, "len >=4, data=%d,%d, %d, %d\n", data[0], data[1], data[2], data[3]);

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

#endif

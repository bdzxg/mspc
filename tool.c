
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>

#include "tool.h"

static char __hex_arr[16] = {'0', '1', '2','3',
	'4','5','6','7','8','9','A','B','C','D','E','F'};

void to_hex(char *data, size_t len, char *des)
{
	size_t i;
	for (i = 0; i < len; i++) {
		uint8_t s = *(data + i);
		*(des + i * 2) 		= __hex_arr[(s & 0xF0) >> 4];
		*(des + i * 2 + 1)  = __hex_arr[s & 0x0F];
	}
}

/*

int main ()
{
	char dest[40] = {0};
	char *src = calloc(1,3);
	*src = 80;
	*(src+1) = 81;

	//printf("%s", src);
	
	to_hex(src, 2, dest);

	printf("%s , len %d\n", dest, strlen(dest));

	return 0;
}
*/

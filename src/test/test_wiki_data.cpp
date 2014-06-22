#include <stdlib.h>
#include "q_util.h"
#include "wiki_data.h"

#define MY_POS (10*1024*1024)

int main(int argc, char *argv[])
{
	mapfile_t mt;
	unsigned int pos = (unsigned int)atoi(argv[2]);
	unsigned int len = sizeof(data_head_t);

	q_mmap(argv[1], &mt);
	
	char *p = (char *)mt.start + sizeof(data_head_t);
	data_record_head_t *h;

	while ((size_t)(p - (char *)mt.start) < mt.size) {
		h = (data_record_head_t *)p;

		if ((int)(len - pos) > (int)MY_POS)
			break;

		if ((int)(pos - len) < (int)MY_POS) {
			printf("pos=%u, out_len=%d\n", len, h->out_len);
		}

		len += sizeof(data_record_head_t) + h->out_len;
		p += sizeof(data_record_head_t) + h->out_len;
	}

	return 0;
}

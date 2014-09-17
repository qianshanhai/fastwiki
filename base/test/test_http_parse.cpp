/*
 * Copyright (C) 2014 Qian Shanhai (qianshanhai@gmail.com)
 */
#include "q_util.h"
#include "http_parse.h"

int main(int argc, char *argv[])
{
	int len;
	char buf[4096];
	mapfile_t mt;

	if (argc < 2) {
		printf("usage: %s <file>\n", argv[0]);
		return 0;
	}

	if (q_mmap(argv[1], &mt) == NULL) {
		perror(argv[1]);
		return 1;
	}

	memset(buf, 0, sizeof(buf));
	len = mt.size >= sizeof(buf) ? sizeof(buf) - 1 : mt.size;

	strncpy(buf, (char *)mt.start, len);

	q_munmap(&mt);

	HttpParse *hp = new HttpParse();

	hp->hp_init_all(buf, len);
	hp->hp_stat();

	return 0;
}

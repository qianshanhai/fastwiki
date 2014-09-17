/*
 * Copyright (C) 2014 Qian Shanhai (qianshanhai@gmail.com)
 */
#include <string.h>
#include <stdlib.h>
#include "q_util.h"
#include "lzo1x.h"

int usage(const char *name)
{
	printf("usage: %s [-d] <from> [to]\n", name);

	return 0;
}

#define _TEST_LZO_FILE_MAX_SIZE (20*1024*1024)

int test_lzo(int flag, const char *from, FILE *to)
{
	int n;
	mapfile_t mt;
	char *buf;

	if (q_mmap(from, &mt) == NULL) {
		perror(from);
		return -1;
	}

	buf = (char *)malloc(_TEST_LZO_FILE_MAX_SIZE);

	LZO1X *lzo = new LZO1X();

	if (flag == 0) 
		n = lzo->compress(buf, _TEST_LZO_FILE_MAX_SIZE, (char *)mt.start, mt.size);
	else
		n = lzo->decompress(buf, _TEST_LZO_FILE_MAX_SIZE, (char *)mt.start, mt.size);

	if (n == -1) {
		printf("lzo error\n");
		return -1;
	}

	fwrite(buf, n, 1, to);
	free(buf);

	delete lzo;

	return 0;
}

int main(int argc, char *argv[])
{
	int de = 0;
	char *from = argv[1];
	FILE *to = stdout;

	if (argc < 2) {
		usage(argv[0]);
		return 0;
	}

	if (strcmp(argv[1], "-d") == 0) {
		de = 1;
		from = argv[2];
		if (argc < 3)
			usage(argv[0]);
	}

	if (de == 0) {
		if (argc > 2)
			to = fopen(argv[2], "w+");
	} else {
		if (argc > 3)
			to = fopen(argv[3], "w+");
	}

	test_lzo(de, from, to);

	fclose(to);

	return 0;
}


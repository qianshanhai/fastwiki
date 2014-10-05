/*
 * Copyright (C) 2014 Qian Shanhai (qianshanhai@gmail.com)
 */
#include <string.h>

#include "q_util.h"
#include "wiki_full_index.h"

int main(int argc, char *argv[])
{
	int total = 0;
	fw_files_t file;

	if (argc < 3) {
		printf("usage: %s <tmpfile> <fidx files> > word.txt\n", argv[0]);
		return 0;
	}

	memset(file, 0, sizeof(file));

	for (int i = 2; i < argc; i++) {
		strcpy(file[total++], argv[i]);
	}

	WikiFullIndex *wfi = new WikiFullIndex();

	if (wfi->wfi_init(file, total) == -1)
		return 1;

	wfi->wfi_stat();
	wfi->wfi_count(argv[1]);

	delete wfi;

	return 0;
}

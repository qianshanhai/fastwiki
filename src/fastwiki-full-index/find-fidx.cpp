/*
 * Copyright (C) 2014 Qian Shanhai (qianshanhai@gmail.com)
 */
#include <string.h>

#include "q_util.h"
#include "wiki_full_index.h"

#define _FW_FULL_INDEX_PREFIX "fastwiki.fidx"

#define _FFI_MAX_TOTAL 64

int main(int argc, char *argv[])
{
	int n = 0, total = 0;
	fw_files_t file;
	int page_idx[_FFI_MAX_TOTAL];

	char string[256];

	if (argc < 3) {
		printf("usage: %s <fidx files> <word>\n", argv[0]);
		return 0;
	}

	memset(file, 0, sizeof(file));
	memset(string, 0, sizeof(string));

	for (int i = 1; i < argc; i++) {
		if (strncmp(argv[i], _FW_FULL_INDEX_PREFIX, sizeof(_FW_FULL_INDEX_PREFIX) - 1) == 0) {
			strcpy(file[total++], argv[i]);
		} else {
			n += sprintf(string + n, "%s ", argv[i]);
		}
	}

	argv[n - 1] = 0;

	WikiFullIndex *wfi = new WikiFullIndex();

	if (wfi->wfi_init(file, total) == -1)
		return 1;

	wfi->wfi_stat();

	total = wfi->wfi_find(string, page_idx, _FFI_MAX_TOTAL);

	printf("total:%d\n", total);

	for (int i = 0; i < total; i++) {
		printf("idx: %d\n", page_idx[i]);
	}

	return 0;
}

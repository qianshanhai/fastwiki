/*
 * Copyright (C) 2014 Qian Shanhai (qianshanhai@gmail.com)
 */

#include <stdio.h>

#include "wiki_scan_file.h"

void print(const struct file_st *p)
{
	printf("lang: %s\n", p->lang);

	for (int i = 0; i < p->data_total; i++)
		printf(" %s\n", p->data_file[i]);

	if (p->index_file[0])
		printf(" %s\n", p->index_file);

	if (p->math_file[0])
		printf(" %s\n", p->math_file);

	for (int i = 0; i < p->image_total; i++)
		printf(" %s\n", p->image_file[i]);

	for (int i = 0; i < p->fidx_file_total; i++)
		printf(" %s\n", p->fidx_file[i]);
}

int main(int argc, char *argv[])
{
	int total;
	fw_dir_t dir[MAX_FD_TOTAL];
	WikiScanFile *wsf;

	if (argc < 2) {
		printf("usgae: %s <dirs ...>\n", argv[0]);
		return 0;
	}
	
	wsf = new WikiScanFile();
	total = wsf->wsf_fetch_fw_dir(dir, MAX_FD_TOTAL, argv + 1, argc - 1);

	for (int i = 0; i < total; i++) {
		printf("%s\n", dir[i].path);
	}

	int lang_total;
	struct file_st file[256];

	lang_total = wsf->wsf_scan_file(file, 256, dir, total);

	for (int i = 0; i < lang_total; i++) {
		struct file_st *p = &file[i];
		print(p);
	}

	delete wsf;

	return 0;
}


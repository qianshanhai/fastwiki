/*
 * Copyright (C) 2014 Qian Shanhai (qianshanhai@gmail.com)
 */
#include "q_util.h"
#include "file_io.h"
#include "fastwiki-dict.h"

FastwikiDict *m_dict;

int add_one_file_title(FastwikiDict *dict, const char *file)
{
	int n;
	char tmp[4096];

	FileIO *file_io = new FileIO();

	file_io->fi_init(file);

	while ((n = file_io->fi_gets(tmp, sizeof(tmp))) > 0) {
		if (strncmp(tmp, "#title:", 7) == 0) {
			chomp(tmp);
			dict->dict_add_title(tmp + 7, n - 7 - 1);
		}
	}
	delete file_io;

	return 0;
}

int add_one_file_page(FastwikiDict *dict, const char *file)
{
	int n, page_len = 0;
	char tmp[4096], *page, curr_title[256];

	FileIO *file_io = new FileIO();

	file_io->fi_init(file);

	page = (char *)malloc(5*1024*1024);

	while ((n = file_io->fi_gets(tmp, sizeof(tmp))) > 0) {
		if (strncmp(tmp, "#title:", 7) == 0) {
			if (page_len > 0) {
				dict->dict_add_page(page, page_len, curr_title, strlen(curr_title));
				page_len = 0;
			}
			chomp(tmp);
			strcpy(curr_title, tmp + 7);
		} else {
			memcpy(page + page_len, tmp, n);
			page_len += n;
		}
	}
	if (page_len > 0) {
		dict->dict_add_page(page, page_len, curr_title, strlen(curr_title));
	}
	delete file_io;

	free(page);

	return 0;
}

int show_usage(const char *name)
{
	printf("Version: 1.0\n");
	printf("Author: qianshanhai\n");
	printf("Usage: fastwiki-text <name> <date> <files ...>\n");

	exit(0);
	return 0;
}

int main(int argc, char *argv[])
{
	if (argc < 3) {
		show_usage(argv[0]);
	}

	for (int i = 3; i < argc; i++) {
		if (!dashf(argv[i])) {
			printf("%s: %s\n", argv[i], strerror(errno));
			return 0;
		}
	}

	m_dict = new FastwikiDict();

	m_dict->dict_init(argv[1], argv[2], "gzip");

	for (int i = 3; i < argc; i++) {
		add_one_file_title(m_dict, argv[i]);
	}

	m_dict->dict_add_title_done();

	for (int i = 3; i < argc; i++) {
		add_one_file_page(m_dict, argv[i]);
	}

	m_dict->dict_output();

	return 0;
}

/*
 * Copyright (C) 2014 Qian Shanhai (qianshanhai@gmail.com)
 */
#include "q_util.h"
#include "file_io.h"
#include "fastwiki-dict.h"

FastwikiDict *m_dict;

static char *text_trim(char *p)
{
	int len;
	trim(p);

	len = strlen(p);

	if (p[0] == p[len - 1] && (p[0] == '\'' || p[0] == '"')) {
		p[len - 1] = 0;
		p++;
	}

	return p;
}

static int parse_title_redirect(const char *str, char *title, char *redirect)
{
	char *p, tmp[1024];

	title[0] = redirect[0] = 0;

	strcpy(tmp, str);

	if ((p = strstr(tmp, "->"))) {
		*p = 0;
		p += 2;
		strcpy(title, text_trim(tmp));
		strcpy(redirect, text_trim(p));
	} else {
		strcpy(title, text_trim(tmp));
	}
	
	return 0;
}

static int add_one_file_title(FastwikiDict *dict, const char *file)
{
	int n;
	char tmp[4096], title[256], redirect[256];

	FileIO *file_io = new FileIO();

	file_io->fi_init(file);

	while ((n = file_io->fi_gets(tmp, sizeof(tmp))) > 0) {
		if (strncmp(tmp, "#title:", 7) == 0) {
			chomp(tmp);
			parse_title_redirect(tmp + 7, title, redirect);
			dict->dict_add_title(title, strlen(title));
		}
	}
	delete file_io;

	return 0;
}

static int add_one_file_page(FastwikiDict *dict, const char *file)
{
	int n, page_len = 0, redirect_len = 0;;
	char tmp[4096], *page, curr_title[256], curr_redirect[256];

	FileIO *file_io = new FileIO();

	file_io->fi_init(file);

	page = (char *)malloc(5*1024*1024);

	while ((n = file_io->fi_gets(tmp, sizeof(tmp))) > 0) {
		if (strncmp(tmp, "#title:", 7) == 0) {
			if (page_len > 0 || redirect_len > 0) {
				dict->dict_add_page(page, page_len, curr_title, strlen(curr_title), curr_redirect, redirect_len);
				page_len = 0;
			}
			chomp(tmp);
			parse_title_redirect(tmp + 7, curr_title, curr_redirect);
			redirect_len = strlen(curr_redirect);
		} else {
			memcpy(page + page_len, tmp, n);
			page_len += n;
		}
	}
	if (page_len > 0 || redirect_len > 0) {
		dict->dict_add_page(page, page_len, curr_title, strlen(curr_title), curr_redirect, redirect_len);
	}
	delete file_io;

	free(page);

	return 0;
}

static int show_usage(const char *name)
{
	printf("Version: %s, %s\n", _VERSION, __DATE__);
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

/*
 * Copyright (C) 2014 Qian Shanhai (qianshanhai@gmail.com)
 */
#include <string.h>
#include <stdlib.h>

#ifdef _WIN32
#include <winsock2.h>
#else
#include <arpa/inet.h>
#endif

#include "q_util.h"

#include "fastwiki-dict.h"

#ifndef _WIN32
extern int script_init(const char *file);
extern int perl_content(char *ret_buf);
#endif

static FastwikiDict *m_dict = NULL;
static char *curr_title = NULL;
static int curr_title_len = 0;
static char *curr_content = NULL;
static int curr_content_len = 0;

int fastwiki_stardict_find_title(const char *title, int len)
{
	return m_dict->dict_find_title(title, len);
}

char *fastwiki_stardict_curr_title(int *len)
{
	*len = curr_title_len;

	return curr_title;
}

char *fastwiki_stardict_curr_content(int *len)
{
	*len = curr_content_len;

	return curr_content;
}

static char *m_buf;

char *page_format(const char *page, int size, int *ret_len, const char *title, int title_len)
{
	int i, len = 0;

	len = sprintf(m_buf, "<b>%s</b> ", title);

	if (page[0] == '*') {
		page++;
		size--;
	}

	for (i = 0; i < size; i++) {
		if (page[i] == '\n') {
			memcpy(m_buf + len, "<br/>\n", 6);
			len += 6;
		} else {
			m_buf[len++] = page[i];
		}
	}

	*ret_len = len;

	return m_buf;
}

struct fw_stardict_st {
	char name[128];
	char perl_file[128];
	char idx[128];
	char dict[128];
	char compress[128];
	int perl_flag;
};

static struct fw_stardict_st m_dict_option;

int show_usage(const char *name)
{
	printf("Version: %s\n", _VERSION);
	printf("Date: %s\n", __DATE__);
	printf("Author: qianshanhai\n");
	printf("Usage: %s <-n name> <-c compress> <-i idx> <-d dict>\n", name);

	exit(0);
	return 0;
}

#define check_option(_x, _y, _n) \
	else if (strcmp(p, _x) == 0 || strcmp(p, _y) == 0) \
		do { \
			if (i >= argc - 1) \
				show_usage(argv[0]); \
			i++; \
			strcpy(st->_n, argv[i]); \
		} while (0)

#define check_option_other() else { show_usage(argv[0]); }

static int init_option(struct fw_stardict_st *st, int argc, char **argv)
{
	memset(st, 0, sizeof(*st));

	if (argc == 1)
		show_usage(argv[0]);

	for (int i = 1; i < argc; i++) {
		char *p = argv[i];
		if (strcmp(p, "-h") == 0 || strcmp(p, "-help") == 0) {
			show_usage(argv[0]);
		}
		check_option("-n", "-name", name);
		check_option("-p", "-perl", perl_file);
		check_option("-i", "-idx", idx);
		check_option("-d", "-dict", dict);
		check_option("-c", "-compress", compress);
		check_option_other();
	}

	if (st->compress[0] == 0)
		strcpy(st->compress, "gzip");

#ifndef _WIN32
	if (dashf(st->perl_file) && script_init(st->perl_file) == 0)
		st->perl_flag = 1;
#endif

	if (!dashf(st->idx)) {
		printf("%s: %s\n", st->idx, strerror(errno));
	}

	if (!dashf(st->dict)) {
		printf("%s: %s\n", st->dict, strerror(errno));
	}

	return 0;
}

int dict_read_file(const char *file, char *buf, int size)
{
	int fd;

#ifdef _WIN32
	if ((fd = open(file, O_RDONLY | O_BINARY)) == -1)
		return -1;
#else
	if ((fd = open(file, O_RDONLY)) == -1)
		return -1;
#endif

	read(fd, buf, size);
	close(fd);

	return 0;
}

int convert_dict(struct fw_stardict_st *st)
{
	int idx_file_size = (int)file_size(st->idx);
	int dict_file_size = (int)file_size(st->dict);

	if (idx_file_size <= 0 || dict_file_size <= 0)
		return 0;

	char *pidx = (char *)malloc(idx_file_size + 1);
	char *pdata = (char *)malloc(dict_file_size + 1);

	dict_read_file(st->idx, pidx, idx_file_size);
	dict_read_file(st->dict, pdata, dict_file_size);

	m_dict = new FastwikiDict();

	m_dict->dict_init(st->name, "20131125", st->compress);

	m_buf = (char *)malloc(5*1024*1024);

	int len;

	for (int pos = 0; pos < idx_file_size ; ) {
		curr_title = pidx + pos;
		len = curr_title_len = strlen(curr_title);

		m_dict->dict_add_title(curr_title, curr_title_len);

		pos += len + 1 + 8;
	}

	m_dict->dict_add_title_done();


	for (int pos = 0; pos < idx_file_size; ) {
		curr_title = pidx + pos;
		len = curr_title_len = strlen(curr_title);

		int index = htonl(*(unsigned int *)(pidx + pos + len + 1));
		int size = htonl(*(unsigned int *)(pidx + pos + len + 5));

		int ret_len;
		char *buf;

		curr_content = pdata + index;
		curr_content_len = size;
		
#ifndef _WIN32
		if (m_dict_option.perl_flag)
		{
			ret_len = perl_content(m_buf);
			buf = m_buf;
		} else
#endif
		{
			buf = page_format(curr_content, curr_content_len, &ret_len, curr_title, curr_title_len);
		}

		m_dict->dict_add_page(buf, ret_len, curr_title, curr_title_len);

		pos += len + 1 + 8;
	}

	m_dict->dict_output();

	return 0;
}

int main(int argc, char *argv[])
{
	init_option(&m_dict_option, argc, argv);

	convert_dict(&m_dict_option);

	return 0;
}

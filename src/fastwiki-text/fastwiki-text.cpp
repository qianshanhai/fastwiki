/*
 * Copyright (C) 2014 Qian Shanhai (qianshanhai@gmail.com)
 */
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>

#include "q_util.h"
#include "q_log.h"
#include "wiki_lua.h"

#include "file_io.h"

#include "fastwiki-dict.h"
#include "fastwiki-text.h"

static FastwikiDict *m_dict;
static WikiLua *m_lua;

#define MAX_ONE_TEXT_FILE (32*1024*1024)

struct ft_option {
	char lang[32];
	char date[32];
	char dir[256];
	char file_type[256];
	split_t sp;
	int total;
	char script_file[256];
	int script_flag;
	char buf[MAX_ONE_TEXT_FILE];
	int buf_len;
	char tmp[MAX_ONE_TEXT_FILE];
	char title[4096];
	int title_len;
};

struct ft_option *m_option = NULL;

void usage(const char *name)
{
	print_usage_head();
	printf("Usage: fastwiki-text <-l lang> <-d date> <-r dir> <-t file type> [-s lua] [-m max_total]\n");
	printf( "       -l lang \n"
			"       -d date \n"
			"       -r dir \n"
			"       -t file type, such as .txt, use ',' to split, \n"
			"          for example: .txt,.htm,.html, etc.\n"
			"       -s lua script file\n"
			"       -m max total. this is debug option, just output article total for -m \n"
			".\n");

	print_usage_tail();
}

#define getopt_str(x, buf) \
	    case x: strncpy(buf, optarg, sizeof(buf) - 1); break

#define getopt_int(x, buf) \
	    case x: buf = atoi(optarg); break

#define getopt_flag(x, buf) \
	    case x: buf = 1; break

static int init_option(struct ft_option *st, int argc, char **argv)
{
	int opt;

	memset(st, 0, sizeof(*st));

	if (argc == 1)
		usage(argv[0]);
while ((opt = getopt(argc, argv, "l:d:r:t:s:m:k")) != -1) {
		switch (opt) {
			getopt_str('l', st->lang);
			getopt_str('d', st->date);
			getopt_str('r', st->dir);
			getopt_str('t', st->file_type);
			getopt_str('s', st->script_file);
			getopt_int('m', st->total);
		}
	}

	if (st->lang[0] == 0 || st->date[0] == 0 || st->dir[0] == 0 || st->file_type[0] == 0)
		usage(argv[0]);

	if (!dashd(st->dir)) {
		printf("Not found dir = %s\n", st->dir);
		usage(argv[0]);
	}

	split(',', st->file_type, st->sp);
	for_each_split(st->sp, T) {
		trim(T);
	}

	if (st->script_file[0] && !dashf(st->script_file)) {
		printf("Not found lua file = %s\n", st->script_file);
		usage(argv[0]);
	}

	m_lua = new WikiLua();
	if (m_lua->lua_init(st->script_file, text_curr_content, text_curr_title, text_find_title) == 0)
		st->script_flag = 1;

	return 0;
}

int text_find_title(const char *title, int len)
{
	return m_dict->dict_find_title(title, len);
}

char *text_curr_title(int *len)
{
	*len = m_option->title_len;

	return m_option->title;
}

char *text_curr_content(int *len)
{
	*len = m_option->buf_len;

	return m_option->buf;
}

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
	
	return strlen(redirect);
}

#define is_title_flag(p) (0[p] == '#' && (1[p] == 't' || 1[p] == 'T') \
		&& (2[p] == 'i' || 2[p] == 'I') && (3[p] == 't' || 3[p] == 'T') \
		&& (4[p] == 'l' || 4[p] == 'L') && (5[p] == 'e' || 5[p] == 'E') && 6[p] == ':') 

int text_add_one_file_title(const char *file, int not_used)
{
	int n;
	char tmp[4096], title[256], redirect[256];

	FileIO *file_io = new FileIO();
	file_io->fi_init(file);

	while ((n = file_io->fi_gets(tmp, sizeof(tmp))) > 0) {
		if (is_title_flag(tmp)) {
			chomp(tmp);
			parse_title_redirect(tmp + 7, title, redirect);
			m_dict->dict_add_title(title, strlen(title));
		}
	}
	delete file_io;

	return 0;
}

/*
 * [[link|title]]  or [[link]]
 */
int text_find_link(const char *tmp, int n, char *out)
{
	char buf[1024];
	const char  *p, *e;
	int len, out_len = 0;

	if ((p = strstr(tmp, "[[")) == NULL) 
		return 0;

	p += 2;

	if ((e = strstr(p, "]]")) == NULL)
		return 0;

	len = e - p;

	if (len >= (int)sizeof(buf))
		return 0;

	memcpy(buf, p, len);
	buf[len] = 0;

	int idx;
	char *sp, *title = buf, *link = buf;

	if ((sp = strchr(buf, '|'))) {
		*sp++ = 0;
		title = sp;
	}

	if ((idx = text_find_title(link, strlen(link))) < 0)
		return 0;

	memcpy(out, tmp, p - 2 - tmp);
	out_len += p - 2 - tmp;

	out_len += sprintf(out + out_len, "<a href='%d#%s'>%s</a>", idx, link, title);

	memcpy(out + out_len, e + 2, tmp + n - (e + 2) + 1);
	out_len += tmp + n - (e + 2) + 1;

	return out_len;
}

int text_add_one_file_page(const char *file, int not_used)
{
	int n, page_len = 0, redirect_len = 0;;
	char tmp[4096], *page, curr_title[256], curr_redirect[256];

	FileIO *file_io = new FileIO();
	file_io->fi_init(file);
	page = (char *)malloc(5*1024*1024);

	while ((n = file_io->fi_gets(tmp, sizeof(tmp) - 1)) > 0) {
		tmp[n] = 0;
		if (is_title_flag(tmp)) {
			if (page_len > 0 || redirect_len > 0) {
				m_dict->dict_add_page(page, page_len, curr_title, strlen(curr_title), curr_redirect, redirect_len);
				page_len = 0;
			}
			chomp(tmp);
			parse_title_redirect(tmp + 7, curr_title, curr_redirect);
			redirect_len = strlen(curr_redirect);
		} else {
			int t;
			if ((t = text_find_link(tmp, n, page + page_len)) > 0) 
				page_len += t;
			else {
				memcpy(page + page_len, tmp, n);
				page_len += n;
			}
		}
	}

	if (page_len > 0 || redirect_len > 0) {
		m_dict->dict_add_page(page, page_len, curr_title, strlen(curr_title), curr_redirect, redirect_len);
	}

	delete file_io;
	free(page);

	return 0;
}

int text_add_one_title(const char *content, int len)
{
	int n;
	char tmp[256], title[256], redirect[256];

	for (int i = 0; i < len; i++) {
		if (is_title_flag(content + i)) {
			for (n = 0, i += 7; i < len && n < (int)sizeof(tmp) - 1; i++) {
				tmp[n++] = content[i];
				if (content[i] == '\n') {
					tmp[n] = 0;
					chomp(tmp);
					parse_title_redirect(tmp, title, redirect);
					m_dict->dict_add_title(title, strlen(title));
					break;
				}
			}
		}
	}

	return 0;
}

int text_add_one_content(const char *content, int len)
{
	int n, page_len = 0, redirect_len = 0;;
	char tmp[256], curr_title[256], curr_redirect[256];
	char *page = m_option->buf;

	for (int i = 0; i < len; i++) {
		if (is_title_flag(content + i)) {
			if (page_len > 0 || redirect_len > 0) {
				m_dict->dict_add_page(page, page_len, curr_title, strlen(curr_title), curr_redirect, redirect_len);
				page_len = 0;
			}
			for (n = 0, i += 7; i < len && n < (int)sizeof(tmp) - 1; i++) {
				tmp[n++] = content[i];
				if (content[i] == '\n') {
					tmp[n] = 0;
					chomp(tmp);
					redirect_len = parse_title_redirect(tmp, curr_title, curr_redirect);
					break;
				}
			}
		} else {
			page[page_len++] = content[i];
		}
	}

	if (page_len > 0 || redirect_len > 0) {
		m_dict->dict_add_page(page, page_len, curr_title, strlen(curr_title), curr_redirect, redirect_len);
	}

	return 0;
}

int fa_parse_fname(const char *file, char *name, char *type)
{
	char *p, *t, buf[256];

	strcpy(buf, file);
	if ((p = strrchr(buf, '.')) == NULL)
		return -1;

	*p++ = 0;

	if ((t = strrchr(buf, '/')))
		t++;
	else
		t= buf;

	strcpy(name, t);
	strcpy(type, p);

	return 0;
}

int fa_match_fname(const char *fname, split_t &sp)
{
	int len = strlen(fname);

	for_each_split(sp, T) {
		int k = strlen(T);
		if (len >= k && strncasecmp(fname + len - k, T, k) == 0)
			return 1;
	}

	return 0;
}

int ft_scan_dir(const char *dir, split_t &sp, int (*func)(const char *file, text_parse_func_t f, int flag),
		text_parse_func_t parse, int flag)
{
	DIR *dirp;
	struct dirent *d;
	char file[256];

	if ((dirp = opendir(dir)) == NULL) {
		LOG("open dir %s: %s\n", dir, strerror(errno));
		return -1;
	}

	while ((d = readdir(dirp))) {
		if (d->d_name[0] == '.')
			continue;

		sprintf(file, "%s/%s", dir, d->d_name);
		if (dashd(file)) {
			ft_scan_dir(file, sp, func, parse, flag);
		} else {
			if (fa_match_fname(d->d_name, sp)) {
				if (func(file, parse, flag) == -1)
					break;
			}
		}
	}

	closedir(dirp);

	return 0;
}

int text_add_one_file(const char *file, text_parse_func_t func, int flag)
{
	int len;
	char title[256], type[64];

	if (fa_parse_fname(file, title, type) == -1)
		return -1;

	if (m_option->script_flag) {
		if ((len = q_read_file(file, m_option->buf, MAX_ONE_TEXT_FILE)) <= 0)
			return -1;

		m_option->buf[len] = 0;

		strcpy(m_option->title, title);
		m_option->title_len = strlen(title);

		m_option->buf_len = len;

		if ((len = m_lua->lua_content(m_option->buf, MAX_ONE_TEXT_FILE, flag)) <= 0)
			return -1;
		m_option->buf[len] = 0;
		
		func(m_option->buf, len);
	} else {
		func(file, 0);
	}


	return 0;
}

int main(int argc, char *argv[])
{
	if (argc < 2) {
		usage(argv[0]);
	}

	text_parse_func_t title_func = NULL, content_func = NULL;

	m_option = (struct ft_option *)malloc(sizeof(struct ft_option));
	init_option(m_option, argc, argv);

	if (m_option->script_flag) {
		title_func = text_add_one_title;
		content_func = text_add_one_content;
	} else {
		title_func = text_add_one_file_title;
		content_func = text_add_one_file_page;
	}

	m_dict = new FastwikiDict();
	m_dict->dict_init(m_option->lang, m_option->date, "gzip");

	ft_scan_dir(m_option->dir, m_option->sp, text_add_one_file, title_func, 1);
	m_dict->dict_add_title_done();

	ft_scan_dir(m_option->dir, m_option->sp, text_add_one_file, content_func, 0);

	m_dict->dict_output();

	return 0;
}

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
#include "wiki_lua.h"
#include "fastwiki-dict.h"
#include "stardict.h"

static FastwikiDict *m_dict = NULL;
static char *curr_title = NULL;
static int curr_title_len = 0;
static char *curr_content = NULL;
static int curr_content_len = 0;

int stardict_find_title(const char *title, int len)
{
	return m_dict->dict_find_title(title, len);
}

char *stardict_curr_title(int *len)
{
	*len = curr_title_len;

	if (curr_title_len == 0)
		return (char *)"";

	return curr_title;
}

char *stardict_curr_content(int *len)
{
	*len = curr_content_len;

	if (curr_content_len == 0)
		return (char *)"";

	return curr_content;
}

static char *m_buf = NULL;

int convert_dict(struct fw_stardict_st *st)
{
	int idx_file_size = (int)file_size(st->idx);
	int dict_file_size = (int)file_size(st->dict);

	if (idx_file_size <= 0 || dict_file_size <= 0)
		return 0;

	char *pidx = (char *)malloc(idx_file_size + 1);
	char *pdata = (char *)malloc(dict_file_size + 1);

	q_read_file(st->idx, pidx, idx_file_size);
	q_read_file(st->dict, pdata, dict_file_size);

	m_dict = new FastwikiDict();

	m_dict->dict_init(st->lang, st->date, st->compress);

	m_buf = (char *)malloc(5*1024*1024);
	WikiLua *lua = new WikiLua();

	if (lua->lua_init(st->script_file, stardict_curr_content, stardict_curr_title, stardict_find_title) == 0) {
		st->script_flag = 1;
	}

	int len;

	for (int pos = 0; pos < idx_file_size ; ) {
		curr_title = pidx + pos;
		len = curr_title_len = strlen(curr_title);

		m_dict->dict_add_title(curr_title, curr_title_len);

		pos += len + 1 + 8;
	}

	m_dict->dict_add_title_done();

	int total = 0;

	for (int pos = 0; pos < idx_file_size; ) {
		total++;
		if (st->max_total > 0 && total > st->max_total)
			break;

		curr_title = pidx + pos;
		len = curr_title_len = strlen(curr_title);

		int index = htonl(*(unsigned int *)(pidx + pos + len + 1));
		int size = htonl(*(unsigned int *)(pidx + pos + len + 5));


		int ret_len;
		char *buf;

		curr_content = pdata + index;
		curr_content_len = size;

		if (st->script_flag) {
			ret_len = lua->lua_content(m_buf);
			buf = m_buf;
		} else {
			ret_len = curr_content_len;
			buf = curr_content;
		}

		m_dict->dict_add_page(buf, ret_len, curr_title, curr_title_len);

		pos += len + 1 + 8;
	}

	m_dict->dict_output();

	return 0;
}


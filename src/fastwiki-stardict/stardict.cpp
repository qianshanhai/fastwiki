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

#include "prime.h"
#include "q_util.h"
#include "s_hash.h"
#include "q_log.h"
#include "crc32sum.h"

#include "wiki_lua.h"
#include "fastwiki-dict.h"
#include "stardict.h"

#define MAX_BUF_SIZE (5*1024*1024)

static FastwikiDict *m_dict = NULL;
static char curr_title[2048];
static int curr_title_len = 0;
static char *curr_content = NULL;
static int curr_content_len = 0;

static SHash *m_tmp_hash = NULL;
static SHash *m_repeat_hash = NULL;

static WikiLua *m_lua = NULL;

static int m_data_fd = -1;
struct fw_stardict_st *m_option = NULL;

struct dict_key {
	char title[64];
};

struct dict_value {
	int total;
};

struct dict_index {
	int flag;
	unsigned index;
	int size;
	int total;
};

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

int add_to_tmp_hash(const char *title, int title_len, unsigned int index, int size)
{
	struct dict_key key;
	struct dict_value *f, value;

	memset(&key, 0, sizeof(key));
	strncpy(key.title, curr_title, sizeof(key.title) - 1);

	if (m_tmp_hash->sh_find(&key, (void **)&f) == _SHASH_FOUND) {
		f->total++;
	} else {
		value.total = 1;
		m_tmp_hash->sh_add(&key, &value);
	}

	return 0;
}

int init_repeat_hash()
{
	struct dict_key key;
	struct dict_value *f;
	struct dict_index idx;

	m_repeat_hash = new SHash();
	m_repeat_hash->sh_set_hash_magic(get_best_hash(100*10000));
	m_repeat_hash->sh_init(10*10000, sizeof(struct dict_key), sizeof(dict_index));

	m_tmp_hash->sh_reset();

	while (m_tmp_hash->sh_read_next(&key, (void **)&f) == _SHASH_FOUND) {
		if (f->total > 1) {
			memset(&idx, 0, sizeof(idx));
			idx.flag = 1;
			m_repeat_hash->sh_add(&key, &idx);
		}
	}

	delete m_tmp_hash;

	return 0;
}

int add_one_title_to_dict(const char *title, int title_len, unsigned int index, int size)
{
	int flag = 1;
	struct dict_key key;
	struct dict_index *fi;

	memset(&key, 0, sizeof(key));
	strncpy(key.title, curr_title, sizeof(key.title) - 1);

	if (m_repeat_hash->sh_find(&key, (void **)&fi) == _SHASH_FOUND) {
		flag = fi->flag;
		fi->flag = 0;
		if (fi->size == 0) {
			fi->size = size;
			fi->index = index;
		} else {
			struct dict_index t;
			t.flag = 0;
			t.size = size;
			t.index = index;
			m_repeat_hash->sh_add(&key, &t);
		}
	}

	if (flag == 1)
		m_dict->dict_add_title(curr_title, curr_title_len);

	return 0;
}

static int m_curr_total = 0;

int add_one_page_to_dict(const char *title, int title_len, unsigned int index, int size)
{
	int ret_len;
	char *buf;
	struct dict_key key;
	struct dict_index *fi;

	if (m_option->max_total> 0 && m_curr_total++ > m_option->max_total)
		return -1;

	memset(&key, 0, sizeof(key));
	strncpy(key.title, title, sizeof(key.title) - 1);

	if (m_repeat_hash->sh_find(&key, (void **)&fi) == _SHASH_FOUND) {
		if (fi->flag == 0) {
			fi->flag = 1;
			void *tmp;
			size = 0;
			m_repeat_hash->sh_begin(&tmp);
			while (m_repeat_hash->sh_next(&tmp, &key, (void **)&fi) == _SHASH_FOUND) {
				lseek(m_data_fd, fi->index, SEEK_SET);
				size += read(m_data_fd, curr_content + size, fi->size);
			}
		} else {
			return 0;
		}
	} else {
		lseek(m_data_fd, index, SEEK_SET);
		read(m_data_fd, curr_content, size);
	}

	curr_content[size] = 0;
	curr_content_len = size;

	if (m_option->script_flag) {
		ret_len = m_lua->lua_content(m_option->buf, MAX_BUF_SIZE);
		buf = m_option->buf;
	} else {
		ret_len = curr_content_len;
		buf = curr_content;
	}

	m_dict->dict_add_page(buf, ret_len, curr_title, curr_title_len);

	return 0;
}

int read_index_file(int fd, int (*func)(const char *title, int title_len, unsigned int index, int size))
{
	int len, size;
	unsigned int index;
	off_t file_size = lseek(fd, 0, SEEK_END);

	if (file_size == (off_t)-1)
		return 0;

	for (int pos = 0; pos < file_size; ) {
		lseek(fd, pos, SEEK_SET);
		read(fd, curr_title, sizeof(curr_title));

		len = curr_title_len = strlen(curr_title);

		lseek(fd, pos + len + 1, SEEK_SET);
		read(fd, &index, sizeof(index));
		read(fd, &size, sizeof(size));

		index = htonl(index);
		size = htonl(size);

		if (func(curr_title, curr_title_len, index, size) == -1)
			break;

		pos += len + 1 + 8;
	}

	return 0;
}

int convert_dict(struct fw_stardict_st *st)
{
	int idx_fd;

	m_option = st;

	if ((idx_fd = open(st->idx, O_RDONLY | O_BINARY)) == -1)
		return -1;

	if ((m_data_fd = open(st->dict, O_RDONLY | O_BINARY)) == -1)
		return -1;

	m_dict = new FastwikiDict();
	m_dict->dict_init(st->lang, st->date, st->compress);

	m_option->buf = (char *)malloc(MAX_BUF_SIZE);
	curr_content = (char *)malloc(MAX_BUF_SIZE);

	m_lua = new WikiLua();
	if (m_lua->lua_init(st->script_file, stardict_curr_content, stardict_curr_title, stardict_find_title) == 0) {
		st->script_flag = 1;
	}

	m_tmp_hash = new SHash();
	m_tmp_hash->sh_set_hash_magic(get_best_hash(1000*10000));
	m_tmp_hash->sh_init(50*10000, sizeof(struct dict_key), sizeof(dict_value));

	read_index_file(idx_fd, add_to_tmp_hash);
	init_repeat_hash();

	read_index_file(idx_fd, add_one_title_to_dict);
	m_dict->dict_add_title_done();

	read_index_file(idx_fd, add_one_page_to_dict);
	m_dict->dict_output();

	return 0;
}


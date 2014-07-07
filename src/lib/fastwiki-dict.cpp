/*
 * Copyright (C) 2014 Qian Shanhai (qianshanhai@gmail.com)
 */
#include <string.h>
#include <stdlib.h>

#include "wiki_common.h"
#include "wiki_data.h"
#include "wiki_index.h"
#include "gzip_compress.h"

#include "fastwiki-dict.h"

static WikiData *m_wiki_data = NULL;
static WikiIndex *m_wiki_index = NULL;
static int m_compress_flag = -1;
static int (*m_compress_func)(char *out, int out_len, const char *in, int in_len);

static char m_data_file[128];
static char m_index_file[128];

static char *m_out = NULL;
static int m_out_len = 8*1024*1024;

FastwikiDict::FastwikiDict()
{
	m_wiki_data = NULL;
	m_wiki_index = NULL;
	m_compress_flag = -1;
	m_compress_func = NULL;

	m_out = NULL;
}

FastwikiDict::~FastwikiDict()
{
	if (m_wiki_index) {
		delete m_wiki_index;
		m_wiki_index = NULL;
	}

	if (m_wiki_data) {
		delete m_wiki_data;
		m_wiki_data = NULL;
	}

	m_compress_flag = -1;
	m_compress_func = NULL;

	if (m_out) {
		free(m_out);
		m_out = NULL;
	}
}

int get_compress_flag(const char *str)
{
	if (strcasecmp(str, "text") == 0 || strcasecmp(str, "txt") == 0)
		return FM_FLAG_TEXT;
	else if (strcasecmp(str, "bzip2") == 0) {
		m_compress_func = bzip2;
		return FM_FLAG_BZIP2;
	} else if (strcasecmp(str, "gzip") == 0) {
		m_compress_func = gzip;
		return FM_FLAG_GZIP;
	} else if (strcasecmp(str, "lz4") == 0) {
		m_compress_func = lz4_compress;
		return FM_FLAG_LZ4;
	}

	return -1;
}

/*
 * compress_flag: lz4, bzip2, gzip, text/txt
 */
int FastwikiDict::dict_init(const char *lang, const char *date, const char *compress_flag)
{
	if ((m_out = (char *)malloc(m_out_len)) == NULL)
		return -1;

	if ((m_compress_flag = get_compress_flag(compress_flag)) == -1)
		return -1;

	sprintf(m_data_file, "fastwiki.dat.%s", lang);
	sprintf(m_index_file, "fastwiki.idx.%s", lang);

	m_wiki_index = new WikiIndex();
	m_wiki_index->wi_output_init();

	m_wiki_data = new WikiData();
	m_wiki_data->wd_init(m_data_file, m_compress_flag, date, lang);

	return 0;
}

int FastwikiDict::dict_add_title(const char *title, int len)
{
	if (len <= 0)
		len = strlen(title);

	return m_wiki_index->wi_one_title(title, len);
}

int FastwikiDict::dict_add_title_done()
{
	return m_wiki_index->wi_add_title_done();
}

int FastwikiDict::dict_add_page(const char *page, int page_size, const char *title, int title_len,
				const char *redirect, int redirect_len)
{
	int data_file_idx = 0, data_len = 0;
	unsigned int data_pos = 0;
	int old_size = page_size;

	if (title_len <= 0)
		title_len = strlen(title);

	if (redirect_len <= 0) {
		if (page_size <= 0 || m_compress_flag == -1)
			return -1;

		if (m_compress_flag != FM_FLAG_TEXT) { 
			int tmp = m_compress_func(m_out, m_out_len, page, page_size);
			if (tmp == -1)
				return -1;
			page = m_out;
			page_size = tmp;
		}
		m_wiki_data->wd_add_one_page(0, page, page_size, old_size,
						&data_file_idx, &data_pos, &data_len);
	}

	m_wiki_index->wi_add_one_page(title, title_len, redirect, redirect_len,
					(unsigned char)data_file_idx, (unsigned short)data_len, data_pos);

	return 0;
}

int FastwikiDict::dict_output()
{
	m_wiki_data->wd_output();
	m_wiki_index->wi_output(m_index_file);

	return 0;
}

int FastwikiDict::dict_find_title(const char *key, int len)
{
	return m_wiki_index->wi_title_find(key, len);
}

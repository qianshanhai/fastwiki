/*
 * Copyright (C) 2014 Qian Shanhai (qianshanhai@gmail.com)
 */

#include <string.h>
#include <stdlib.h>

#include "q_util.h"

#include "wiki_index.h"
#include "wiki_data.h"
#include "wiki_full_index.h"

static SHash *m_word_hash = NULL;

int load_word(const char *file)
{
	FILE *fp;
	char buf[1024];
	struct wfi_tmp_key key;
	
	if ((fp = fopen(file, "r")) == NULL)
		return -1;

	while (fgets(buf, sizeof(buf), fp)) {
		chomp(buf);
		memset(&key, 0, sizeof(key));

		strncpy(key.word, buf, sizeof(key.word) - 1);
		m_word_hash->sh_replace(&key, NULL);
	}

	fclose(fp);

	return 0;
}

static WikiIndex *m_wiki_index = NULL;
static WikiData *m_wiki_data = NULL;
static WikiFullIndex *m_full_index = NULL;

static const int max_page_size = 5*1024*1024;

static int m_href_curr = -1;
static int m_href_total;
static pthread_mutex_t m_fidx_mutex;

static int wiki_fetch_one_page(int *href_idx, char *buf, int buf_len, int *ret_len)
{
	int n;
	sort_idx_t idx;

	pthread_mutex_lock(&m_fidx_mutex);

	for (;;) {
		m_href_curr++;
		if (m_href_curr >= m_href_total)
			break;

		if (m_wiki_index->wi_fetch(m_href_curr, &idx)) {
			if ((n = m_wiki_data->wd_sys_read((int)idx.data_file_idx,
							idx.data_pos, idx.data_len, buf, buf_len)) > 0) {
				buf[n] = 0;
				*href_idx = m_href_curr;
				*ret_len = n;
				pthread_mutex_unlock(&m_fidx_mutex);
				return 1;
			}
		}
	}

	pthread_mutex_unlock(&m_fidx_mutex);

	return 0;
}

static void *wiki_full_index_thread(void *arg)
{
	int idx, n;
	char *buf = (char *)malloc(max_page_size);
	int pthread_idx = *(int *)arg;

	while (wiki_fetch_one_page(&idx, buf, max_page_size, &n)) {
		m_full_index->wfi_add_page(idx, buf, n, pthread_idx);
	}

	return NULL;
}

int start_create_index(const char *index_file, fw_files_t data_file, int total,
		const char *tmp_dir, size_t mem_size, const char *z_flag)
{
	sort_idx_t idx;
	data_head_t data_head;

	pthread_mutex_init(&m_fidx_mutex, NULL);

	m_wiki_index = new WikiIndex();
	m_wiki_index->wi_init(index_file);

	m_href_total = m_wiki_index->wi_href_total();

	m_wiki_data = new WikiData();
	m_wiki_data->wd_init(data_file, total);
	m_wiki_data->wd_get_head(&data_head);

	int page_total = 0;

	for (int i = 0; i < m_href_total; i++) {
		if (m_wiki_index->wi_fetch(i, &idx)) {
			page_total++;
		}
	}

	m_full_index  = new WikiFullIndex();

	int max_thread = wiki_pthread_total();

	m_full_index->wfi_create_init(z_flag, page_total + 1, data_head.lang, tmp_dir, mem_size, m_word_hash, max_thread);

	pthread_t id[128];

	for (int i = 0; i < max_thread; i++) {
		int n = pthread_create(&id[i], NULL, wiki_full_index_thread, &i);
		if (n != 0){
			return -1;
		}
		q_sleep(1);
	}

	for (int i = 0; i < max_thread; i++) {
		pthread_join(id[i], NULL);
	}

	m_full_index->wfi_add_page_done();

	return 0;
}

int main(int argc, char *argv[])
{
	char word_file[128], *z_flag = argv[1];
	char index_file[128];
	fw_files_t data_file;

	int data_file_total = 0;

	if (argc < 5) {
		printf("Version: %s %s, %s\n", _VERSION, __DATE__, __TIME__);
		printf("Usage: %s <z_flag> <zh.txt> <index_file> <data_files>\n", argv[0]);
		return 0;
	}

	strcpy(word_file, argv[2]);

	memset(index_file, 0, sizeof(index_file));

	for (int i = 3; i < argc; i++) {
		if (strncmp(argv[i], "fastwiki.idx", 12) == 0)
			strcpy(index_file, argv[i]);
		else if (strncmp(argv[i], "fastwiki.dat", 12) == 0)
			strcpy(data_file[data_file_total++], argv[i]);
	}

	if (data_file_total == 0 || index_file[0] == 0)
		return 1;

	m_word_hash = new SHash();
	m_word_hash->sh_init(10*10000, sizeof(struct wfi_tmp_key), 0);

	if (load_word(word_file) == -1) {
		printf("load file %s error\n", word_file);
		return 1;
	}

	start_create_index(index_file, data_file, data_file_total, "tmp", 2*1024*1024*1024L, z_flag);

	return 0;
}

/*
 * Copyright (C) 2014 Qian Shanhai (qianshanhai@gmail.com)
 */

#include <string.h>
#include <stdlib.h>

#include "q_util.h"

#include "wiki_index.h"
#include "wiki_data.h"
#include "wiki_full_index.h"
#include "wiki_scan_file.h"

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

typedef struct {
	char dir[128];
	char word_file[128];
	long long mem_size;
	char tmp[128];
	int pthread_total;
} ffi_arg_t;

ffi_arg_t m_ffi_arg;

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

	int max_thread = m_ffi_arg.pthread_total;
	
	if (max_thread == 0)
		max_thread = wiki_pthread_total();

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

int usage(const char *name)
{
	printf("Version: %s, %s %s\n", _VERSION, __DATE__, __TIME__);
	printf("Author: Qianshanhai\n");
	printf("usage: fastwiki-full-index <-d dir> [-w word] [-m mem size] [-t tmp] [-p pthread total]\n");
	printf( "       -d  folder that include fastwiki data files.\n"
			"       -w  mutil-byte language word list\n"
			"       -m  memory size, default is 1024MB\n"
			"       -t  temporary folder, default is 'tmp'\n"
			"       -p  pthread total, default is host cpu total\n"
			".\n"
			);

	exit(0);
	return 0;
}

int ffi_init_option(int argc, char *argv[])
{
	int opt;
	ffi_arg_t *p = &m_ffi_arg;

	memset(p, 0, sizeof(ffi_arg_t));

	while ((opt = getopt(argc, argv, "hd:w:m:t:p:")) != -1) {
		switch (opt) {
			case 'h':
				usage(argv[0]);
				break;
			case 'd':
				strncpy(p->dir, optarg, sizeof(p->dir) - 1);
				break;
			case 'w':
				strncpy(p->word_file, optarg, sizeof(p->word_file) - 1);
				break;
			case 'm':
				p->mem_size = atoll(optarg);
				break;
			case 't':
				strncpy(p->tmp, optarg, sizeof(p->tmp) - 1);
				break;
			case 'p':
				p->pthread_total = atoi(optarg);
				break;
			default:
				break;
		}
	}

	if (p->tmp[0] == 0)
		strcpy(p->tmp, "tmp");

	if (q_mkdir(p->tmp, 0755) == -1) {
		printf("error: mkdir %s: %s\n", p->tmp, strerror(errno));
		exit(0);
	}

	if (p->mem_size == 0)
		p->mem_size = 1024*1024*1024L;

	if (p->dir[0] == 0)
		usage(argv[0]);

	return 0;
}

int main(int argc, char *argv[])
{
	fw_dir_t dir;
	WikiScanFile *wsf;
	struct file_st file[10];
	ffi_arg_t *ff = &m_ffi_arg;

	ffi_init_option(argc, argv);
	strcpy(dir.path, ff->dir);

	wsf = new WikiScanFile();
	int total = wsf->wsf_scan_file(file, sizeof(file) / sizeof(struct file_st), &dir, 1);

	if (total > 1) {
		printf("found many language data in this folder.\n");
		exit(0);
	}

	if (total == 0) {
		printf("Not found any fastwiki idx file and dat files in this folder\n");
		printf("run 'fastwiki-full-index -h'  to see help\n");
		exit(0);
	}

	m_word_hash = new SHash();
	m_word_hash->sh_init(10*10000, sizeof(struct wfi_tmp_key), 0);

	if (ff->word_file[0]) {
		if (load_word(ff->word_file) == -1) {
			printf("load file %s error\n", ff->word_file);
			return 1;
		}
	}

	start_create_index(file[0].index_file, file[0].data_file,
			file[0].data_total, ff->tmp, ff->mem_size, "gzip");

	return 0;
}

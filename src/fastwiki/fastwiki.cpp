/*
 * Copyright (C) 2014 Qian Shanhai (qianshanhai@gmail.com)
 */
#include <stdlib.h>
#include <stdio.h>
#include <sys/stat.h>
#include <errno.h>
#include <dirent.h>
#include <pthread.h>

#include "crc32sum.h"
#include "q_util.h"
#include "my_strstr.h"
#include "wiki_data.h"
#include "wiki_index.h"

#include "wiki_parse.h"
#include "file_read.h"
#include "wiki_common.h"

#define print(...) \
	do { \
		printf(__VA_ARGS__);  \
		fflush(stdout); \
	} while (0)

WikiData *m_wiki_data = NULL;
WikiIndex *m_wiki_index = NULL;
FileRead *m_file_read = NULL;

WikiParse *m_wiki_parse = NULL;

pthread_mutex_t m_key_mutex;
pthread_mutex_t m_page_mutex;

time_t m_curr;

typedef struct {
	int zh_flag;
	char lang[32];
	char date[32];
	char wiki_file[128];
	char creator[128];
	int split_size;
} fw_arg_t;

fw_arg_t m_fw_arg;

int find_key_hash(const char *key, int len)
{
	int n;

	pthread_mutex_lock(&m_key_mutex);
	n = m_wiki_index->wi_title_find(key, len);
	pthread_mutex_unlock(&m_key_mutex);

	return n;
}

int add_page(int flag, const char *page, int page_z_size, int page_old_size,
		const char *_title, int len, const char *redirect, int n)
{
	int data_file_idx;
	int data_len;
	unsigned int data_pos;

	if (n <= 0) {
		pthread_mutex_lock(&m_page_mutex);

		m_wiki_data->wd_add_one_page(flag, page, page_z_size, page_old_size,
				&data_file_idx, &data_pos, &data_len);

		pthread_mutex_unlock(&m_page_mutex);
	}

	pthread_mutex_lock(&m_key_mutex);
	m_wiki_index->wi_add_one_page(_title, len, redirect, n,
				(unsigned char)data_file_idx, (unsigned short)data_len, data_pos);
	pthread_mutex_unlock(&m_key_mutex);

	return 0;
}

int add_title(int flag, const char *page, int page_z_size, int page_old_size,
		const char *_title, int len, const char *redirect, int n)
{
	return m_wiki_index->wi_one_title(_title, len);
}

int init_key_hash(const char *file)
{
	char *buf;
	int size;
	struct wiki_parse wp;

	FileRead *fr;

	m_wiki_index = new WikiIndex();
	m_wiki_index->wi_output_init();

	fr = new FileRead();
	if (fr->fr_init(file) == -1)
		return -1;

	m_wiki_parse->wp_init(add_title, find_key_hash, 0, 0);

	memset(&wp, 0, sizeof(wp));
	wp.parse_flag = PARSE_TITLE_FLAG;
	buf = (char *)malloc(ONE_BLOCK_SIZE + ONE_BLOCK_SIZE_PAD);

	while (1) {
		if (fr->fr_get_one_block(buf, &size) <= 0)
			break;
		m_wiki_parse->wp_do(buf, size, &wp);
	}

	delete fr;
	free(buf);

	m_wiki_index->wi_add_title_done();

	return 0;
}

int init_var()
{
	fw_arg_t *p = &m_fw_arg;

	m_wiki_parse = new WikiParse();
	if (init_key_hash(p->wiki_file) == -1) {
		return -1;
	}

	m_wiki_parse->wp_init(add_page, find_key_hash, p->zh_flag, FM_FLAG_GZIP);

	pthread_mutex_init(&m_key_mutex, NULL);
	pthread_mutex_init(&m_page_mutex, NULL);

	char fname[64];
	sprintf(fname, "fastwiki.dat.%s", p->lang);

	m_wiki_data = new WikiData();
	m_wiki_data->wd_init(fname, FM_FLAG_GZIP, p->date, p->lang);

	return 0;
}

#define MAX_TMP_SIZE (5*1024*1024)

static void *wiki_read_thread(void *arg)
{
	char *buf = (char *)malloc(ONE_BLOCK_SIZE + ONE_BLOCK_SIZE_PAD);
	int size;
	struct wiki_parse wp;

	memset(&wp, 0, sizeof(wp));

	wp.ref_buf = (char *)malloc(MAX_TMP_SIZE);
	wp.tmp = (char *)malloc(MAX_TMP_SIZE);
	wp.zip = (char *)malloc(MAX_TMP_SIZE);
	wp.math = (char *)malloc(MAX_TMP_SIZE);
	wp.chapter = (char *)malloc(MAX_TMP_SIZE);

	wp.ref_count = wp.ref_size = 0;
	wp.parse_flag = PARSE_ALL_FLAG;

	while (1) {
		if (m_file_read->fr_get_one_block(buf, &size) <= 0)
			break;
		m_wiki_parse->wp_do(buf, size, &wp);
	}

	free(wp.ref_buf);
	free(wp.tmp);
	free(wp.zip);
	free(wp.math);
	free(wp.chapter);
	free(buf);

#ifdef DEBUG
	print("exit\n");
	fflush(stdout);
#endif

	return NULL;
}

int count_use_time(const char *file, int max_thread)
{
	off_t size = file_size(file);

	size = (unsigned long)size / 1024 / 220 / 60 / max_thread;

	if (size == 0)
		size = 1;

	return size;
}


int use_of_time()
{
	int s = (int)(time(NULL) - m_curr);
	int h = s / 3600;
	int m = (s - h * 3600) / 60;

	s = s - h * 3600 - m * 60;

	print("use of time: %02d:%02d:%02d\r\n", h, m, s);

	return 0;
}

int start_convert()
{
	int max_thread, minutes;
	
	max_thread = q_get_cpu_total();
	minutes = count_use_time(m_fw_arg.wiki_file, max_thread);

	print("Convert will take about %d minutes.\r\n", minutes);
	print("  --> Depending on your machine configuration.\r\n\r\n");

	init_var();

	pthread_t id[128];

	m_file_read = new FileRead();
	m_file_read->fr_init(m_fw_arg.wiki_file);

	for (int i = 0; i < max_thread; i++) {
		int n = pthread_create(&id[i], NULL, wiki_read_thread, NULL);
		if (n != 0){
			return -1;
		}
		q_sleep(1);
	}

	for (int i = 0; i < max_thread; i++) {
		pthread_join(id[i], NULL);
	}

	m_wiki_data->wd_output();

	char fname_list[1024], index_fname[128];

	sprintf(index_fname, "fastwiki.idx.%s", m_fw_arg.lang);

	m_wiki_data->wd_file_list(fname_list);
	m_wiki_index->wi_output(index_fname);

	delete m_wiki_data;
	use_of_time();

	print("output file: \r\n%s\t%s\r\n\r\n", fname_list, index_fname);

	return 0;
}

void usage(const char *name)
{
	print_usage_head();

	printf("usage: fastwiki <-l lang> <-d date> <-f file> [-z] [-c creator] [-s size] \n");
	printf( "       -l language string, used to distinguish other data files.\n"
			"       -d date, such as 201312 \n"
			"       -f wikipedia raw file.\n"
			"       -z convert all Traditional Chinese to Simplified Chinese,\n"
			"          If doubt, don't assign -z\n"
			"       -c user, You name, you are author of this data files.\n"
			"       -s size. split files size, default is 1800MB,\n"
			"          If doubt, don't assign -s\n"
			".\n"
		  );

	print_usage_tail();
}

int fw_init_option(int argc, char **argv)
{
	int opt;
	fw_arg_t *p = &m_fw_arg;

	memset(p, 0, sizeof(fw_arg_t));

	while ((opt = getopt(argc, argv, "zl:d:c:f:s:")) != -1) {
		switch (opt) {
			case 'z':
				p->zh_flag = 1;
				break;
			case 'l':
				strncpy(p->lang, optarg, sizeof(p->lang) - 1);
				break;
			case 'd':
				strncpy(p->date, optarg, sizeof(p->date) - 1);
				break;
			case 'f':
				strncpy(p->wiki_file, optarg, sizeof(p->wiki_file) - 1);
				break;
			case 'c':
				strncpy(p->creator, optarg, sizeof(p->creator) - 1);
				break;
			case 's':
				p->split_size = atoi(optarg);
				break;
			default:
				break;
		}
	}

	if (p->split_size > 10*1024*1024 && p->split_size <= 2000*1000*1000)
		set_wiki_split_size(p->split_size);

	if (p->lang[0] == 0 || p->date[0] == 0 || p->wiki_file[0] == 0)
		usage(argv[0]);

	if (!dashf(p->wiki_file)) {
		printf("open file %s to read error: %s\n", p->wiki_file, strerror(errno));
		exit(0);
	}

	return 0;
}

int main(int argc, char *argv[])
{
	fw_init_option(argc, argv);

	time(&m_curr);
	start_convert();

	return 0;
}

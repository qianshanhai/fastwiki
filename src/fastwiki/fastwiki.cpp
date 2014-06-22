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

char m_lang[512];

char m_page_file[256];
char m_date[128];
time_t m_curr;

typedef char fw_regex_t[16][96];

const char *z_compress_str[] = {
	"zlib -- default compress algorithm",
	"lz4  -- fast compress algorithm, and save power, but size increase 20%",
	NULL
};

const int z_compress_value[] = {
	FM_FLAG_GZIP,
	FM_FLAG_LZ4,
};

int z_compress_flag = z_compress_value[0];

#include <regex.h>

int fw_regex(const char *match, const char *str, fw_regex_t ret)
{
	regmatch_t buf[64];
	regex_t reg;

	memset(buf, 0, sizeof(buf));

	if (regcomp(&reg, match,  REG_EXTENDED | REG_ICASE) != 0)
		return -1;

	if (regexec(&reg, str, 64, buf, REG_EXTENDED | REG_ICASE) != 0) {
		regfree(&reg);
		return -1;
	}

	for (int i = 0; i < 16 && buf[i].rm_so >= 0; i++) {
		strncpy(ret[i], str + buf[i].rm_so, buf[i].rm_eo - buf[i].rm_so);
	}

	regfree(&reg);

	return 0;
}

int fw_parse_filename(const char *fname)
{
	char *p, tmp[256];
	fw_regex_t buf;

	memset(m_lang, 0, sizeof(m_lang));
	memset(m_date, 0, sizeof(m_date));

	memset(buf, 0, sizeof(buf));

	if (fw_regex("(.*)(wik.*)-([0-9]{8})-(pages-article)", fname, buf) == -1)
		return -1;

	if (strcasecmp(buf[2], "wiki") == 0)
		strcpy(tmp, buf[1]);
	else
		sprintf(tmp, "%s%s", buf[1], buf[2] + 3);

	if ((p = strrchr(tmp, '/'))) {
		*p++ = 0;
		strcpy(m_lang, p);
	} else {
		strcpy(m_lang, tmp);
	}

	strcpy(m_date, buf[3]);

	return 0;
}

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

int init_var(int zh_flag)
{
	m_wiki_parse = new WikiParse();
	if (init_key_hash(m_page_file) == -1) {
		return -1;
	}

	m_wiki_parse->wp_init(add_page, find_key_hash, zh_flag, z_compress_flag);

	pthread_mutex_init(&m_key_mutex, NULL);
	pthread_mutex_init(&m_page_mutex, NULL);

	char fname[64];
	sprintf(fname, "fastwiki.dat.%s", m_lang);

	m_wiki_data = new WikiData();
	m_wiki_data->wd_init(fname, z_compress_flag, m_date, m_lang);


	return 0;
}

#define MAX_TMP_SIZE (3*1024*1024)

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

int get_fname(char *page_file, char *date)
{
	DIR *dirp;
	struct dirent *d;
	int total = 0;
	char file[16][100];

	if ((dirp = opendir(".")) == NULL)
		return -1;

	while ((d = readdir(dirp))) {
		if (d->d_name[0] == '.')
			continue;
		if (fw_parse_filename(d->d_name) == 0) {
			strcpy(file[total++], d->d_name);
			if (total > 15)
				break;
		}
	}
	closedir(dirp);

	if (total == 0)
		return 0;

	if (total == 1) {
		strcpy(page_file, file[0]);
	} else {
		print("Found many Wiki Dump files, please select one file:\r\n");
		print("(发现太多的维基文件, 请选择一个):\r\n");
		for (int i = 0; i < total; i++) {
			print("[%d] %s\r\n", i + 1, file[i]);
		}
		for (;;) {
			print("Input an index number, range is 1-%d. (输入编号, 范围是 1-%d):", total, total);
			char buf[64];
			read(STDIN_FILENO, buf, sizeof(buf));
			if (atoi(buf) >= 1 && atoi(buf) <= total) {
				strcpy(page_file, file[atoi(buf) - 1]);
				print("\r\n");
				break;
			}
		}
	}
	fw_parse_filename(page_file);
	print("Use Wiki Dump file: %s\r\n", page_file);
	print("(使用维基原始文件 : %s)\r\n", page_file);

	return 0;
}

int get_pages_article_from_argv(char *argv[])
{
	char *p;
	int n = 0;

	strcpy(m_page_file, argv[1]);

	if ((p = strrchr(m_page_file, '/')))
		n = fw_parse_filename(p + 1);
	else
		n = fw_parse_filename(m_page_file);

	return n;
}

int get_pages_article()
{
	m_page_file[0] = 0;

	get_fname(m_page_file, m_date);

	if (m_page_file[0] == 0)
		return -1;

	return 0;
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

	print("use of time(转换时间): %02d:%02d:%02d\r\n", h, m, s);

	return 0;
}

void fw_quit()
{
	if (wiki_is_dont_ask()) {
		exit(0);
	}

	print("Press Ctrl-C to exit(按 Ctrl-C 退出) ...\r\n");

	while (1) {
		q_sleep(3600);
	}
	print("\r\n");
	exit(0);
}

int use_zh_convert()
{
	int flag = 0;

	if (wiki_is_dont_ask())
		return 0;

	if (strncmp(m_lang, "zh", 2) != 0)
		return 0;
	
	print("\r\n");
	print("转成简体中文? (Convert to simple zh?) [y/N]: ");

	char buf[128];
	fgets(buf, sizeof(buf), stdin);
	chomp(buf);

	if (buf[0] == 'y' || buf[0] == 'Y') {
		print("所有内容将转成简体中文(All content will convert to simple zh).\r\n");
		flag = 1;
	} else {
		print("不对内容进行任何简繁转换.(Don't use all zh convert function)\r\n");
	}

	return flag;
}

int start_convert()
{
	int zh_flag, max_thread, minutes;
	
	zh_flag = use_zh_convert();
	max_thread = q_get_cpu_total();
	minutes = count_use_time(m_page_file, max_thread);

	print("Convert will take about %d minutes.(整个转换大概需要 %d 分钟)\r\n", minutes, minutes);
	print("  --> Depending on your machine configuration.(视您的机器配置而定)\r\n\r\n");

	/* in init_var() maybe change m_page_file */
	init_var(zh_flag);

	pthread_t id[128];

	m_file_read = new FileRead();
	m_file_read->fr_init(m_page_file);

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

	sprintf(index_fname, "fastwiki.idx.%s", m_lang);

	m_wiki_data->wd_file_list(fname_list);
	m_wiki_index->wi_output(index_fname);

	delete m_wiki_data;
	use_of_time();

	print("output file(输出文件): \r\n%s\t%s\r\n\r\n", fname_list, index_fname);

	return 0;
}

void print_version()
{
	char author[128];

	print("--- Fastwiki ----\r\nVersion: %s\r\n", _VERSION);

	author[0] = 'A';
	author[1] = 'u';
	author[2] = 't';
	author[3] = 'h';
	author[4] = 'o';
	author[5] = 'r';
	author[6] = ':';
	author[7] = ' ';
	author[8] = 'q';
	author[9] = 'i';
	author[10] = 'a';
	author[11] = 'n';
	author[12] = 's';
	author[13] = 'h';
	author[14] = 'a';
	author[15] = 'n';
	author[16] = 'h';
	author[17] = 'a';
	author[18] = 'i';
	author[19] = 0;

	print("%s\r\n", author);

	print("Publish Date: %s\r\n\r\n", __DATE__);

}

int fw_input_file()
{
	char buf[1024];

	if (wiki_is_dont_ask())
		return -1;

	while (1) {
		print("Input wiki dump file(输入维基原始文件): ");
		fflush(stdout);
		read(STDIN_FILENO, buf, sizeof(buf));
		chomp(buf);
		if (buf[0] == 0)
			return -1;
		if (!dashf(buf)) {
			print("File not exist(文件不存在): %s\r\n", buf);
		} else {
			strcpy(m_page_file, buf);
			fw_parse_filename(m_page_file);

			if (m_date[0] == 0) {
				print("Input wiki date, format is 2013MMDD(输入维基日期, 格式是 2013MMDD): ");
				read(STDIN_FILENO, buf, sizeof(buf));
				chomp(buf);
				strncpy(m_date, buf, 12);
				m_date[12] = 0;
			}

			if (m_lang[0] == 0) {
				print("Input language name, such as 'en'(输入语言标识, 例如 'en'): ");
				read(STDIN_FILENO, buf, sizeof(buf));
				chomp(buf);
				strncpy(m_lang, buf, 8);
				m_lang[8] = 0;
			}

			break;
		}
	}

	return 0;
}

void fw_set_split_size()
{
	char buf[16];

	if (wiki_is_dont_ask())
		return;

	print("\r\n");
	print("Chinese: 需要显示高级选项吗? 通常这是不需要的.\r\n");
	print("Show Advanced Options? Usually is not needed. [y/N]:");
	read(STDIN_FILENO, buf, sizeof(buf));
	if (!(buf[0] == 'y' || buf[0] == 'Y'))
		return;

	print("\r\nSet split size, default is 1800MB.(默认是按 1800M 分割输出数据文件)[y/N]:");
	read(STDIN_FILENO, buf, sizeof(buf));

	if (buf[0] == 'y' || buf[0] == 'Y') {
		for (;;) {
			print("Please input a size, units is MB(输入分割大小, 单位是 MB): ");
			memset(buf, 0, sizeof(buf));
			read(STDIN_FILENO, buf, sizeof(buf));
			chomp(buf);
			int n = atoi(buf);
			if (n <= 1)
				continue;

			if (n > 10)
				break;

			print("Your input '%d' is too small, it will output many many files.\r\n", n);
			print("你输入的 '%d' 太小了, 会产生非常多的文件.\r\n", n);
		}
		set_wiki_split_size(atoi(buf));
	}

	print("Set output compress algorithm? [y/N]:");
	read(STDIN_FILENO, buf, sizeof(buf));

	if (buf[0] == 'y' || buf[0] == 'Y') {
		for (;;) {
			int i;
			for (i = 0; z_compress_str[i]; i++) {
				print("%d. %s\n", i + 1, z_compress_str[i]);
			}
			print("Please pick up an algorithm [1-%d]: ", i);
			read(STDIN_FILENO, buf, sizeof(buf));
			int n = atoi(buf);
			if (n > 0 && n <= i) {
				z_compress_flag = z_compress_value[n - 1];
				break;
			}
		}
	}
}	

int main(int argc, char *argv[])
{
	int n;

	print_version();

	if (argc > 1) {
		n = get_pages_article_from_argv(argv);
	} else {
		n = get_pages_article();
	}

	if (n == -1) {
		print("Not found any Wiki Dump files in current folder.\r\n");
		print("(当前目录没有发现维基百科的原始文件.)\r\n");
		if (fw_input_file() == -1)
			fw_quit();
	}

	if (!dashf(m_page_file)) {
		print("Not found Wiki Dump File(没有发现维基文件)...\r\n");
		fw_quit();
	}

	fw_set_split_size();
	time(&m_curr);
	start_convert();

	fw_quit();

	return 0;
}

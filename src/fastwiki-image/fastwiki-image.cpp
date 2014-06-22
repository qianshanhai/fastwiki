/*
 * Copyright (C) 2014 Qian Shanhai (qianshanhai@gmail.com)
 */
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>

#include "wiki_curl.h"
#include "image_parse.h"
#include "image_tmp.h"

#ifndef __IMAGE_NOT_OUT
#include "wiki_image.h"
#endif

#include "wiki_common.h"

#include "q_util.h"
#include "s_hash.h"

ImageParse *m_image_parse = NULL;
WikiCurl   *m_wiki_curl = NULL;
ImageTmp *m_image_tmp = NULL;

#ifndef __IMAGE_NOT_OUT
WikiImage *m_wiki_image = NULL;
#endif

SHash *m_hash = NULL;
FILE *m_fp = NULL;

pthread_mutex_t m_ip_mutex;
pthread_mutex_t m_it_add_mutex;
pthread_mutex_t m_file_mutex;
pthread_mutex_t m_hash_mutex;

int m_all_total = 0;
int m_read_total = 0;
int m_error_total = 0;

struct save_data {
	char fname[1024];
	char link[2048];
	int flag;
};

struct chdup_key {
	char fname[512];
};

struct chdup_value {
	int v;
};

int m_sig_int_flag = 0;

void sig_int(int sig)
{
	m_sig_int_flag = 1;
}

#define FI_TMP_IMAGE_OLD "/dev/shm/fi1.old.svg"
#define FI_TMP_IMAGE_NEW "/dev/shm/fi1.new.jpg"
#define FI_TMP_IMAGE_TMP "/dev/shm/fi1.new.tmp.jpg"

#define FI_CONVERT_1 "convert " FI_TMP_IMAGE_OLD " " FI_TMP_IMAGE_NEW
#define FI_CONVERT_2 "convert -resize 800x640 " FI_TMP_IMAGE_TMP  " " FI_TMP_IMAGE_NEW

int fi_convert_svg2jpg(char *buf, int *size)
{
	int len = *size;

	if (strncasecmp(buf, "<?xml", 5) != 0)
		return 0;

	unlink(FI_TMP_IMAGE_OLD);
	unlink(FI_TMP_IMAGE_NEW);

	file_append(FI_TMP_IMAGE_OLD, buf, len);

	system(FI_CONVERT_1);
	
	int new_size = (int)file_size(FI_TMP_IMAGE_NEW);

	if (new_size > 512*1024) {
		rename(FI_TMP_IMAGE_NEW, FI_TMP_IMAGE_TMP);
		system(FI_CONVERT_2);
	}
	
	int fd = open(FI_TMP_IMAGE_NEW, O_RDONLY);
	if (fd == -1)
		return -1;

	new_size = (int)file_size(fd);

	read(fd, buf, new_size);
	close(fd);

	*size = new_size;

	return 0;
}

int get_file_line(const char *file)
{
	FILE *fp = fopen(file, "r");

	if (fp == NULL)
		return 0;

	char buf[2048];
	int total = 0;

	while (fgets(buf, sizeof(buf), fp))
		total++;

	fclose(fp);

	return total;
}


int fi_init_curl()
{
	char *env, buf[1024] = {0};
	m_wiki_curl = new WikiCurl();

	m_wiki_curl->curl_init();

	printf("Set Proxy?[y/N]: ");
	fgets(buf, sizeof(buf), stdin);

	chomp(buf);

	if (strcasecmp(buf, "y") == 0) {
		if ((env = getenv("HTTP_PROXY"))) {
			strcpy(buf, env);
			printf("use HTTP_PROXY env: %s\n", buf);
		} else {
			printf("Input Proxy: ");
			fgets(buf, sizeof(buf), stdin);
		}
		chomp(buf);
		m_wiki_curl->curl_set_proxy(buf);
	}

	return 0;
}

#define TEST_DIR "test"
#define IMAGE_FNAME "filename.txt"

int fi_init_image_parse()
{
	m_image_parse = new ImageParse();
	m_image_parse->ip_init();

	m_image_tmp = new ImageTmp();
	m_image_tmp->it_init(TEST_DIR);

#ifndef __IMAGE_NOT_OUT
	m_wiki_image = new WikiImage();
#endif

	m_hash = new SHash();
	m_hash->sh_init(TEST_DIR "/fastwiki.image.chdup",
			sizeof(struct chdup_key), sizeof(struct chdup_value), 300000, 1);

	m_read_total = m_hash->sh_hash_total();
	m_all_total = get_file_line(IMAGE_FNAME);

	printf("%.2f%%, all=%d, done=%d\n", 
					(float)(m_read_total) * 100.0 / (float)m_all_total,
					m_all_total, m_read_total);

	pthread_mutex_init(&m_ip_mutex, NULL);
	pthread_mutex_init(&m_it_add_mutex, NULL);
	pthread_mutex_init(&m_file_mutex, NULL);
	pthread_mutex_init(&m_hash_mutex, NULL);

	if ((m_fp = fopen(IMAGE_FNAME, "r")) == NULL) {
		printf("open fname.txt error\n");
		return -1;
	}

	return 0;
}

int fi_format(char *s, int len)
{
	int pos = 0;
	char tmp[4096];

	for (int i = 0; i < len; i++) {
		if (strncmp(s + i, "&amp;", 5) == 0) {
			tmp[pos++] = '&';
			i += 4;
		} else if (strncmp(s + i, "&quot;", 6) == 0) {
			tmp[pos++] = '"';
			i += 5;
		} else if (s[i] == ' ')
			tmp[pos++] = '_';
		else
			tmp[pos++] = s[i];
	}

	tmp[pos] = 0;
	strcpy(s, tmp);

	return pos;
}

int fi_check_dup(const struct chdup_key *key)
{
	int n;
	struct chdup_value *f;

	pthread_mutex_lock(&m_hash_mutex);
	n = m_hash->sh_find(key, (void **)&f);
	pthread_mutex_unlock(&m_hash_mutex);

	return n == _SHASH_FOUND ? 1 : 0;
}

int fi_add_hash(const struct chdup_key *key)
{
	struct chdup_value v;

	v.v = 1;

	pthread_mutex_lock(&m_hash_mutex);

	m_hash->sh_add(key, &v);
	m_read_total++;

	pthread_mutex_unlock(&m_hash_mutex);

	return 0;
}

int fi_read_error()
{
	pthread_mutex_lock(&m_hash_mutex);

	m_error_total++;

	pthread_mutex_unlock(&m_hash_mutex);

	return 0;
}

int fi_do_one_url(int idx, const char *tmp)
{
	char line[1024], url[1024], image[1024];
	struct chdup_key key;
	int n;

	char *data;
	int len;

	strcpy(line, tmp);
	ltrim(line);

	fi_format(line, strlen(line));

	for (int i = strlen(line) - 1; i >= 0; i--) {
		if ((line[i] >= 'a' && line[i] <= 'z') || (line[i] >= 'A' && line[i] <= 'Z'))
			break;
		line[i] = 0;
	}

	memset(&key, 0, sizeof(key));
	strncpy(key.fname, line, sizeof(key.fname) - 1);

	if (fi_check_dup(&key))
		return 1;

	sprintf(url, "http://zh.wikipedia.org/wiki/File:%s", line);
	//sprintf(url, "http://elderscrolls.wikia.com/wiki/File:%s", line);

	if (m_wiki_curl->curl_get_data(idx, url, &data, &len) > 0) {

		pthread_mutex_lock(&m_ip_mutex);

		m_image_parse->ip_parse(data, len);
		n = m_image_parse->ip_get_url(image);

		pthread_mutex_unlock(&m_ip_mutex);
		
		if (n) {
			sprintf(url, "http:%s", image);
			if (m_wiki_curl->curl_get_data(idx, url, &data, &len) > 0) {

				pthread_mutex_lock(&m_it_add_mutex);
				m_image_tmp->it_add(tmp, data, len);
				pthread_mutex_unlock(&m_it_add_mutex);

				fi_add_hash(&key);

				return 0;
			}
		}
	}

	fi_read_error();

	return -1;
}

int fi_output_image_with_file_list(const char *lang, const char *file)
{
	FILE *fp;
	char line[1024];
	char *buf = (char *)malloc(5*1024*1024);
	int len;

	if ((fp = fopen(file, "r")) == NULL) {
		perror(file);
		return -1;
	}

	m_wiki_image->we_init(lang, "20121001");

	while (fgets(line, sizeof(line), fp)) {
		chomp(line);
		if (m_sig_int_flag) {
			printf("catch int\n");
			break;
		}
		if (m_image_tmp->it_find(line, buf, &len) == 0) {
			format_image_name(line, sizeof(line));
			if (strncasecmp(line + strlen(line) - 4, ".svg", 4) == 0) {
				fi_convert_svg2jpg(buf, &len);
			}
			m_wiki_image->we_add_one_image(line, buf, len);
		}
	}

	char fname[2048];

	m_wiki_image->we_output(fname);

	fclose(fp);

	return 0;
}


int fi_output_image(const char *lang)
{
#ifndef __IMAGE_NOT_OUT
	char fname[1024];
	char *buf = (char *)malloc(10*1024*1024);
	int size;

	m_wiki_image->we_init(lang, "20121001");

	m_image_tmp->it_reset();
	while (m_image_tmp->it_read_next(fname, buf, &size)) {
		if (m_sig_int_flag) {
			printf("catch int\n");
			break;
		}
		format_image_name(fname, sizeof(fname));
		if (strncasecmp(fname + strlen(fname) - 4, ".svg", 4) == 0) {
			fi_convert_svg2jpg(buf, &size);
		}
		m_wiki_image->we_add_one_image(fname, buf, size);
#if 0
		if (total++ > 1000)
			break;
#endif
	}

	m_wiki_image->we_output(fname);
#endif

	return 0;
}

int fi_get_one_url(char *buf)
{
	char tmp[1024];

	pthread_mutex_lock(&m_file_mutex);

	if (fgets(tmp, sizeof(tmp), m_fp)) {
		chomp(tmp);
		strcpy(buf, tmp);
		pthread_mutex_unlock(&m_file_mutex);
		return 0;
	}

	pthread_mutex_unlock(&m_file_mutex);

	return -1;
}

static void *fetch_image_thread(void *arg)
{
	int idx = *((int *)arg);
	char buf[1024];
	int n;

	while (1) {
		if (m_sig_int_flag) {
			printf("catch int\n");
			break;
		}
		if (fi_get_one_url(buf) == -1)
			break;

		if ((n = fi_do_one_url(idx, buf)) == 0) {;;
			printf("%d %.2f%%, all=%d, done=%d, error=%d\n",
					idx,
					(float)(m_read_total) * 100.0 / (float)m_all_total,
					m_all_total, m_read_total, m_error_total);
		} else if (n == -1) {
			printf("%d error:%s\n", idx, buf);
		}
	}

	return NULL;
}

int fi_start_fetch_thread(int max_thread)
{
	pthread_t id[128];

	for (int i = 0; i < max_thread; i++) {
		int idx = i;
		int n = pthread_create(&id[i], NULL, fetch_image_thread, &idx);
		if (n == -1) {
			printf("error pthread\n");
			exit(0);
		}

		q_sleep(1);
	}

	for (int i = 0; i < max_thread; i++) {
		pthread_join(id[i], NULL);
	}

	return 0;
}

int main(int argc, char *argv[])
{
	fi_init_curl();
	fi_init_image_parse();

	q_signal(SIGINT, sig_int);

	if (!(argc > 1 && strcasecmp(argv[1], "create") == 0)) {
		printf("start fetch all image ...\n");
		fi_start_fetch_thread(argc > 1 ? atoi(argv[1]) : 1);
	}

	if (m_sig_int_flag == 0) {
		printf("output image ...\n");
		if (argc > 2 && dashf(argv[2]))
			fi_output_image_with_file_list("zh", argv[2]);
		else
			fi_output_image("zh");
	}

	return 0;
}

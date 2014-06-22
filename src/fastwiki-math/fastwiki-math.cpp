/*
 * Copyright (C) 2014 Qian Shanhai (qianshanhai@gmail.com)
 */
#include <stdlib.h>

#include "file_read.h"
#include "wiki_math.h"
#include "wiki_common.h"

static WikiMath *m_wiki_math;
static FileRead *m_file_read;
static pthread_mutex_t m_add_mutex;

int my_math_parse(char *buf, int size, char *tmp)
{
	int n;
	char *start;

	buf[size] = 0;

	for (int i = 0; i < size; i++) {
out:
		if (buf[i] == '&') {
			if (strncmp(buf + i + 1, "lt;math&gt;", 11) == 0) {
				i += 12;
				start = buf + i;
				for (int j = 0; j < size - i; j++) {
					if (start[j] == '&') {
						if (strncmp(start + j + 1, "lt;/math", 8) == 0 
								|| strncmp(start + j + 1, "lt;math/", 8) == 0) {
							start[j] = 0;
							n = j;
							if (n < 512*1024) {
								math_format(start, &n, tmp);
								if (n > 0) {
									pthread_mutex_lock(&m_add_mutex);
									m_wiki_math->wm_add(start, n);
									pthread_mutex_unlock(&m_add_mutex);
								}
							}
							i += j + 8;
							goto out;
						}
					}
				}
			}
		}
	}

	return 0;
}

static void *wiki_read_thread(void *arg)
{
	int size;
	char *buf = (char *)malloc(ONE_BLOCK_SIZE + ONE_BLOCK_SIZE_PAD);
	char *tmp = (char *)malloc(ONE_BLOCK_SIZE + ONE_BLOCK_SIZE_PAD);

	while (1) {
		if (m_file_read->fr_get_one_block(buf, &size) == 0)
			break;
		my_math_parse(buf, size, tmp);
	}

	return NULL;
}

void print_math_version()
{
	printf("--- Fastwiki Math convert tool ----\n"
			"Version: %s\n"
			"Author: qianshanhai\n"
			"Publish Date: %s\n\n", _VERSION, __DATE__);
}

int main(int argc, char *argv[])
{
	pthread_t id[128];
	int max_thread;
	int flag = -1;

	if (argc > 1)
		flag = atoi(argv[1]);

	print_math_version();

	if (init_texvc_file() == -1) {
		printf("Not found texvc, please install texvc.\n");
		printf("And must have those command: latex, dvipng \n");
		return 1;
	}

	if (argc < 4 || flag < 0 || flag > 2) {
		printf("usage: %s <flag> <input> <output>\n", argv[0]);
		printf("       flag: \n"
				"         0  input: wiki dump file, output: math-tmpfile \n"
				"         1  input: math-tmpfile ,  output: fastwiki math file\n"
				"         2  input: wiki dump file, output: fastwiki math file\n"
				" flag is debug option, please set flag=2, example: \n"
				"     ./fastwiki-math 2 zhwiki-20131009-pages-article.xml.bz2 fastwiki.math.zh \n");
		
		return 0;
	}

	max_thread = q_get_cpu_total();

	pthread_mutex_init(&m_add_mutex, NULL);

	m_wiki_math = new WikiMath();
	if (flag == 0)
		m_wiki_math->wm_init_parse(flag, argv[3]);
	else if (flag == 1) 
		m_wiki_math->wm_init_parse(flag, argv[2]);
	else if (flag == 2)
		m_wiki_math->wm_init_parse(flag);

	if (flag == 0 || flag == 2) {
		m_file_read = new FileRead();
		m_file_read->fr_init(argv[2]);

		for (int i = 0; i < max_thread; i++) {
			int n = pthread_create(&id[i], NULL, wiki_read_thread, NULL);
			if (n != 0){
				return -1;
			}
			usleep(30000);
		}

		for (int i = 0; i < max_thread; i++) {
			pthread_join(id[i], NULL);
		}
	}

	if (flag == 1 || flag == 2)
		m_wiki_math->wm_output(argv[3], max_thread);

	return 0;
}

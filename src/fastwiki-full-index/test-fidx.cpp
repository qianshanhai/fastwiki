/*
 * Copyright (C) 2014 Qian Shanhai (qianshanhai@gmail.com)
 */

#include <string.h>
#include <stdlib.h>

#include "q_util.h"
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

		//printf("%s\n", buf);
		strncpy(key.word, buf, sizeof(key.word) - 1);
		m_word_hash->sh_replace(&key, NULL);
	}

	fclose(fp);

	return 0;
}

int main(int argc, char *argv[])
{
	if (argc < 3) {
		printf("usage: %s <zh.txt> <files>\n", argv[0]);
		return 0;
	}

	m_word_hash = new SHash();
	m_word_hash->sh_init(10*10000, sizeof(struct wfi_tmp_key), 0);

	if (load_word(argv[1]) == -1) {
		printf("load file %s error\n", argv[1]);
		return 1;
	}

	WikiFullIndex *wfi = new WikiFullIndex();

	wfi->wfi_create_init("gzip", 100, "zh", "tmp", 1*1024*1024, m_word_hash);

	FILE *fp;
	char *p, buf[1024];

	if ((fp = fopen(argv[2], "r")) == NULL) {
		perror(argv[2]);
		return 1;
	}

	while (fgets(buf, sizeof(buf), fp)) {
		chomp(buf);
		if ((p = strchr(buf, ' '))) {
			*p++ = 0;
			wfi->wfi_add_page(atoi(buf), p, strlen(p));
		}
	}

	wfi->wfi_add_page_done();

	return 0;
}

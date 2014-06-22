/*
 * Copyright (C) 2014 Qian Shanhai (qianshanhai@gmail.com)
 */
#ifndef __WIKI_MEMORY_H
#define __WIKI_MEMORY_H

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <pthread.h>

typedef struct {
	char *addr;
	int size;
	int block_idx;
	int mem_idx;
} wiki_mem_t;

#define MAX_BLOCK_TOTAL 1024
#define MAX_BLOCK_REMOVE 1024

struct wiki_mem_block {
	char *buf;
	int curr_use;
	int remove[MAX_BLOCK_REMOVE];
	int remove_total;
};

class WikiMemory {
	private:
		int m_block_total;
		int m_max_total;
		int m_one_size;

		pthread_mutex_t m_mutex;
		struct wiki_mem_block *m_mem_block;

	public:
		WikiMemory();
		~WikiMemory();

		int w_mem_init(int one_size, int max_total);
		int w_mem_new_one_block();

		int w_mem_malloc(wiki_mem_t *wm);
		int w_mem_free(wiki_mem_t *wm);
		int w_mem_array(wiki_mem_t *wm, char **p);
};

#endif

/*
 * Copyright (C) 2014 Qian Shanhai (qianshanhai@gmail.com)
 */
#include "wiki_memory.h"

WikiMemory::WikiMemory()
{
}

WikiMemory::~WikiMemory()
{
}

int WikiMemory::w_mem_init(int one_size, int max_total)
{
	m_one_size = one_size;
	m_max_total = max_total;

	if (one_size * max_total > 1024*1024*1024)
		return -1;

	m_block_total = 0;
	m_mem_block = (struct wiki_mem_block *)calloc(MAX_BLOCK_TOTAL, sizeof(struct wiki_mem_block));

	memset(m_mem_block, 0, MAX_BLOCK_TOTAL * sizeof(struct wiki_mem_block));
	w_mem_new_one_block();

	pthread_mutex_init(&m_mutex, NULL);

	return 0;
}

int WikiMemory::w_mem_new_one_block()
{
	struct wiki_mem_block *bl = &m_mem_block[m_block_total++];

#ifdef DEBUG
	printf("new block = %d\n", m_block_total);
#endif

	bl->buf = (char *)calloc(m_max_total, m_one_size);
	memset(bl->buf, 0, sizeof(m_max_total * m_one_size));

	bl->remove_total = 0;
	bl->curr_use = 0;

	for (int i = 0; i < MAX_BLOCK_REMOVE; i++) {
		bl->remove[i] = -1;
	}

	return 0;
}

int WikiMemory::w_mem_malloc(wiki_mem_t *wm)
{
	struct wiki_mem_block *bl;

	if (pthread_mutex_lock(&m_mutex) == -1) {
		printf("\x1b[31merror\x1b[m\n");
		exit(0);
	}

	for (int i = 0; i < m_block_total; i++) {
		bl = &m_mem_block[i];
		if (bl->remove_total > 0) {
			wm->block_idx = i;
			wm->mem_idx = bl->remove[bl->remove_total - 1];
			wm->addr = bl->buf + wm->mem_idx * m_one_size;
			wm->size = m_one_size;

			bl->remove_total--;
			pthread_mutex_unlock(&m_mutex);

			return 0;
		}
		if (bl->curr_use < m_max_total) {
			wm->block_idx = i;
			wm->mem_idx = bl->curr_use;
			wm->addr = bl->buf + bl->curr_use * m_one_size;
			wm->size = m_one_size;

			bl->curr_use++;
			pthread_mutex_unlock(&m_mutex);

			return 0;
		}
	}

	w_mem_new_one_block();

	bl = &m_mem_block[m_block_total - 1];

	wm->block_idx = m_block_total - 1;
	wm->mem_idx = 0;
	wm->addr = bl->buf;
	wm->size = m_one_size;

	bl->curr_use++;

	pthread_mutex_unlock(&m_mutex);

	return 0;
}

int WikiMemory::w_mem_free(wiki_mem_t *wm)
{
	struct wiki_mem_block *bl;

	if (pthread_mutex_lock(&m_mutex) == -1) {
		printf("\x1b[31merror\x1b[m\n");
		exit(0);
	}

	bl = &m_mem_block[wm->block_idx];
	bl->remove[bl->remove_total] = wm->mem_idx;
	bl->remove_total++;

	memset(wm, 0, sizeof(wiki_mem_t));

	pthread_mutex_unlock(&m_mutex);

	return 0;
}

/*
 * Not used
 */
int WikiMemory::w_mem_array(wiki_mem_t *wm, char **p)
{
	int size = m_one_size / 1024;

	for (int i = 0; i < 1024; i++) {
		p[i] = wm->addr + i * size;
	}

	return 0;
}

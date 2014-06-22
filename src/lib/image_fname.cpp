/*
 * Copyright (C) 2014 Qian Shanhai (qianshanhai@gmail.com)
 */
#include <pthread.h>

#include "prime.h"
#include "q_util.h"
#include "crc32sum.h"

#include "image_fname.h"

struct if_key {
	unsigned int crc32;
	unsigned int r_crc32;
	int len;
};

ImageFname::ImageFname()
{
	m_hash = NULL;
	m_fd = -1;
}

ImageFname::~ImageFname()
{
	if (m_fd >= 0)
		close(m_fd);

	if (m_hash) {
		m_hash->sh_destroy();
		delete m_hash;
	}
}

int ImageFname::if_init(const char *file)
{
	m_hash = new SHash();

	m_hash->sh_set_hash_magic(get_best_hash(100*10000));
	if (m_hash->sh_init(100*10000, sizeof(struct if_key), 0) == -1)
		return -1;

	m_fd = open(file, O_RDWR | O_CREAT | O_APPEND | O_TRUNC, 0644);

	pthread_mutex_init(&m_mutex, NULL);

	return 0;
}

int ImageFname::if_add_fname(const char *fname)
{
	struct if_key k;
	char buf[1024];

	k.len = snprintf(buf, sizeof(buf), "%s\n", fname);

	crc32sum(buf, k.len - 1, &k.crc32, &k.r_crc32);

	pthread_mutex_lock(&m_mutex);

	if (m_hash->sh_find(&k, NULL) == _SHASH_FOUND) {
		pthread_mutex_unlock(&m_mutex);
		return 0;
	}

	m_hash->sh_add(&k, NULL);
	write(m_fd, buf, k.len);

	pthread_mutex_unlock(&m_mutex);

	return 0;
}

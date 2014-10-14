/*
 * Copyright (C) 2014 Qian Shanhai (qianshanhai@gmail.com)
 */

#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>

#include "q_log.h"
#include "q_util.h"
#include "prime.h"

#include "wiki_audio.h"

WikiAudio::WikiAudio()
{
	m_max_size = 0;
	m_fd = -1;
	m_init_flag = 0;
}

WikiAudio::~WikiAudio()
{
	if (m_fd >= 0)
		close(m_fd);
}

int WikiAudio::wa_stat()
{
	LOG("hash_pos: %d\n", m_head.hash_pos);

	return 0;
}

int WikiAudio::wa_init(const char *file)
{
	if ((m_fd = open(file, O_RDONLY | O_BINARY)) == -1) {
		LOG("open file %s error: %s\n", file, strerror(errno));
		return -1;
	}

	read(m_fd, &m_head, sizeof(m_head));

	m_hash = new SHash();
	m_hash->sh_fd_init_ro(m_fd, m_head.hash_pos);

	m_init_flag = 1;

	return 0;
}

int WikiAudio::wa_find(const char *title, char *data, int *len, int max_size)
{
	unsigned int pos;
	int size;

	if (wa_find_pos(title, &pos, &size) == -1)
		return -1;

	if (size > max_size)
		return -2;

	lseek(m_fd, pos, SEEK_SET);
	read(m_fd, data, size);

	*len = size;

	return 0;
}

int WikiAudio::wa_find_pos(const char *title, unsigned int *pos, int *len)
{
	struct audio_key key;
	struct audio_value v;

	if (m_init_flag == 0)
		return -1;

	memset(&key, 0, sizeof(key));
	strncpy(key.title, title, sizeof(key.title) - 1);

	q_tolower(key.title);

	if (m_hash->sh_fd_find(&key, &v) != _SHASH_FOUND) 
		return -1;

	*pos = v.pos;
	*len = v.len;

	return 0;
}

int WikiAudio::wa_get_fd()
{
	return m_fd;
}

int WikiAudio::wa_init_rw(int max_total, const char *outfile)
{
	m_max_total = max_total;

	m_hash = new SHash();
	m_hash->sh_set_hash_magic(get_best_hash(max_total + 1024));
	m_hash->sh_init(max_total, sizeof(struct audio_key), sizeof(struct audio_value));

	if ((m_fd = open(outfile, O_RDWR | O_CREAT | O_BINARY | O_TRUNC, 0644)) == -1) {
		LOG("open file %s to write error: %s\n", outfile, strerror(errno));
		return -1;
	}

	int size;
	void *addr;

	m_hash->sh_get_addr(&addr, &size);

	write(m_fd, &m_head, sizeof(m_head));
	write(m_fd, addr, size);

	m_file_pos = sizeof(m_head) + size;

	return 0;
}

int WikiAudio::wa_add_one_audio(const char *title, const char *data, int len, const char *type)
{
	struct audio_key key;
	struct audio_value value, *f;

	if (m_hash->sh_hash_total() >= m_max_total)
		return 0;

	memset(&key, 0, sizeof(key));
	memset(&value, 0, sizeof(value));

	strncpy(key.title, title, sizeof(key.title) - 1);
	q_tolower(key.title);

	if (m_hash->sh_find(&key, (void **)&f) == _SHASH_FOUND)
		return 0;

	if (len > m_max_size)
		m_max_size = len;

	value.pos = m_file_pos;
	value.len = len;
	strncpy(value.type, type, sizeof(value.type) - 1);

	m_hash->sh_add(&key, &value);

	write(m_fd, data, len);
	m_file_pos += len;

	return 0;
}

int WikiAudio::wa_output()
{
	int size;
	void *addr;

	m_hash->sh_get_addr(&addr, &size);

	memset(&m_head, 0, sizeof(m_head));

	m_head.hash_pos = sizeof(m_head);
	m_head.hash_size = size;
	m_head.total = m_hash->sh_hash_total();
	m_head.max_size = m_max_size;

	lseek(m_fd, 0, SEEK_SET);
	write(m_fd, &m_head, sizeof(m_head));
	write(m_fd, addr, size);

	return 0;
}

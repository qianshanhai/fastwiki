/*
 * Copyright (C) 2014 Qian Shanhai (qianshanhai@gmail.com)
 */
#include "prime.h"
#include "wiki_image.h"

#ifdef WIN32
#define pthread_mutex_init(x, y) 
#define pthread_mutex_lock(x) 
#define pthread_mutex_unlock(x) 
#endif

WikiImage::WikiImage()
{
	m_hash = NULL;
	m_fd_hash = NULL;
	m_init_flag = -1;

	for (int i = 0; i < _MAX_IMAGE_FILE_TOTAL; i++)
		m_fd[i] = -1;

	m_image_total = 0;
}

WikiImage::~WikiImage()
{
	if (m_hash)
		delete m_hash;

	if (m_fd_hash)
		delete m_fd_hash;

	for (int i = 0; i < _MAX_IMAGE_FILE_TOTAL; i++) {
		if (m_fd[i] >= 0)
			close(m_fd[i]);
	}
}

int WikiImage::we_init(const fw_files_t file, int total)
{
	int fd, flag = 0;
	struct wiki_image_head head;

	m_image_total = 0;

	if (total <= 0)
		return -1;

	for (int i = 0; i < total; i++) {
		if ((fd = open(file[i], O_RDONLY | O_BINARY)) == -1) {
			perror(file[i]);
			return -1;
		}
		read(fd, &head, sizeof(head));
		m_fd[head.file_idx] = fd;

		m_image_total += head.total;

		if (head.file_idx == 0) {
			/* index in files of serial 0 */
			if (head.hash_size == 0) {
				printf("error1:%d, i = %d\n", head.hash_size, i);
				return -1;
			}
			m_fd_hash = new SHash();
			m_fd_hash->sh_fd_init_ro(fd, sizeof(head) + head.data_size);
			flag = 1;
		}
	}

	if (flag == 0) {
#ifdef DEBUG
		printf("Not found first image file.\n");
#endif
		return -1;
	}

	pthread_mutex_init(&m_mutex, NULL);

	memset(&m_value, 0, sizeof(m_value));

	for (int i = 0; i < _MAX_PTHREAD_TOTAL; i++) {
		m_file_pos[i] = -1;
	}

	m_init_flag = 0;

#ifdef DEBUG
	printf("image total=%d\n", m_image_total);
#endif

	return 0;
}

int WikiImage::we_fd_reset()
{
	return m_fd_hash->sh_fd_reset();
}

int WikiImage::we_fd_read_next(void *key, void *value)
{
	return m_fd_hash->sh_fd_read_next(key, value);
}

int WikiImage::we_reset(int pthread_idx, const char *fname, int *size)
{
	int n;
	struct wiki_image_key key;
	struct wiki_image_value *f = &m_value[pthread_idx];

	if (m_init_flag == -1)
		return -1;

	m_file_pos[pthread_idx] = -1;

	memset(&key, 0, sizeof(key));

	strncpy(key.fname, fname, sizeof(key.fname) - 1);

	pthread_mutex_lock(&m_mutex);

	if ((n = m_fd_hash->sh_fd_find(&key, f)) == _SHASH_FOUND) {
		m_file_pos[pthread_idx] = 0;
		*size = f->data_len;
	}

	pthread_mutex_unlock(&m_mutex);

	if (f->file_idx < 0 || f->file_idx >= _MAX_IMAGE_FILE_TOTAL)
		return -1;

	return (n == _SHASH_FOUND && m_fd[f->file_idx] >= 0) ? 0 : -1;
}

#define ONE_R_IMAGE (128*1024)

#define pread we_pread

int WikiImage::we_pread(int fd, void *data, int len, unsigned int pos)
{
	int n;

	pthread_mutex_lock(&m_mutex);

	lseek(fd, pos, SEEK_SET);
	n = read(fd, data, len);

	pthread_mutex_unlock(&m_mutex);

	return n;
}

int WikiImage::we_read_next(int pthread_idx, char *data, int *len)
{
	struct wiki_image_value *p = &m_value[pthread_idx];
	int tmp, fd;
	
	if (m_file_pos[pthread_idx] == -1 || m_file_pos[pthread_idx] >= p->data_len) {
		m_file_pos[pthread_idx] = -1;
		return 0;
	}

	fd = m_fd[p->file_idx];

	int remain = p->data_len - m_file_pos[pthread_idx];

	if (remain > ONE_R_IMAGE) {
		tmp = pread(fd, data, ONE_R_IMAGE,
				sizeof(struct wiki_image_head) + p->data_pos + m_file_pos[pthread_idx]);
		m_file_pos[pthread_idx] += ONE_R_IMAGE;
		*len = tmp;
	} else if (remain > 0) {
		tmp = pread(fd, data, remain, 
				sizeof(struct wiki_image_head) + p->data_pos + m_file_pos[pthread_idx]);
		m_file_pos[pthread_idx] += remain;
		*len = tmp;
	}

	return 1;
}

#ifndef FW_NJI

int WikiImage::we_init(const char *lang, const char *date)
{
	m_hash = new SHash();
	m_hash->sh_set_hash_magic(get_max_prime(30*10000));
	m_hash->sh_init(20*10000, sizeof(struct wiki_image_key), sizeof(struct wiki_image_value));

	m_curr_fd = -1;
	m_curr_file_idx = -1;
	m_curr_size = 0;
	m_lang = lang;

	memset(m_file_name, 0, sizeof(m_file_name));

	we_new_file();

	return 0;
}

#define _MAX_IMAGE_FILE_SIZE (1990*1024*1024)

int WikiImage::we_add_one_image(const char *fname, const char *data, int len)
{
	if (m_curr_size > _MAX_IMAGE_FILE_SIZE) {
		we_add_done();
		we_new_file();
	}

	struct wiki_image_key key;
	struct wiki_image_value value;

	memset(&key, 0, sizeof(key));
	memset(&value, 0, sizeof(value));

	strncpy(key.fname, fname, sizeof(key.fname) - 1);

	m_head.total++;

	value.file_idx = m_curr_file_idx;
	value.data_pos = m_curr_size;
	value.data_len = len;

	m_hash->sh_add(&key, &value);

	m_curr_size += len;

	lseek(m_curr_fd, 0, SEEK_END);
	write(m_curr_fd, data, len);

	return 0;
}

int WikiImage::we_output(char *buf)
{
	we_add_done();
	we_write_hash();

	if (buf)
		strcpy(buf, m_file_name);

	return 0;
}

char *WikiImage::we_get_file_name(char *file, int file_idx)
{
	sprintf(file, "fastwiki.image.%s.%d", m_lang, file_idx);

	return file;
}

int WikiImage::we_new_file()
{
	char file[128];

	m_curr_file_idx++;
	we_get_file_name(file, m_curr_file_idx);

	if ((m_curr_fd = open(file, O_RDWR | O_TRUNC | O_CREAT | O_BINARY, 0644)) == -1)
		return -1;

	memset(&m_head, 0, sizeof(m_head));
	m_head.flag = 0xfa87;

	write(m_curr_fd, &m_head, sizeof(m_head));

	strcat(m_file_name, file);
	strcat(m_file_name, "\n");

	return 0;
}

int WikiImage::we_add_done()
{
	m_head.file_idx = m_curr_file_idx;
	m_head.data_size = m_curr_size;
	m_head.hash_size = 0;

	lseek(m_curr_fd, 0, SEEK_SET);
	write(m_curr_fd, &m_head, sizeof(m_head));

	m_curr_size = 0;
	close(m_curr_fd);

	printf("total[%d] = %d\n", m_curr_file_idx, m_head.total);

	return 0;
}

int WikiImage::we_write_hash()
{
	char file[128];
	void *addr;
	int fd, size;

	we_get_file_name(file, 0);

	if ((fd = open(file, O_RDWR | O_BINARY, 0644)) == -1) {
		perror(file);
		return -1;
	}

	read(fd, &m_head, sizeof(m_head));

	m_hash->sh_get_addr(&addr, &size);
	m_head.hash_size = size;

	lseek(fd, 0, SEEK_SET);
	write(fd, &m_head, sizeof(m_head));

	lseek(fd, 0, SEEK_END);
	write(fd, addr, size);

	close(fd);

	return 0;
}

#endif

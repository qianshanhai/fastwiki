/*
 * Copyright (C) 2014 Qian Shanhai (qianshanhai@gmail.com)
 */
#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>

#include "q_util.h"
#include "image_tmp.h"

ImageTmp::ImageTmp()
{
	m_curr_fd = -1;
}

ImageTmp::~ImageTmp()
{
	if (m_hash)
		delete m_hash;
}

int ImageTmp::it_init(const char *dir)
{
	m_file_idx = 0;
	m_curr_size = 0;

	if (!dashd(dir))
		q_mkdir(dir, 0755);

	strncpy(m_dir, dir, sizeof(m_dir) - 1);

	char file[128];

	sprintf(file, "%s/%s.idx", dir, IMAGE_TMP_PREFIX);

	m_hash = new SHash();
	m_hash->sh_init(file, sizeof(struct image_tmp_key), sizeof(struct image_tmp_value), 25*10000, 1);

	if (it_get_last_fname() == 0)
		return 0;

	return 0;
}

int ImageTmp::it_find(const char *fname, char *data, int *len)
{
	struct image_tmp_key key;
	struct image_tmp_value *f;

	memset(&key, 0, sizeof(key));

	strncpy(key.name, fname, sizeof(key.name) - 1);

	if (m_hash->sh_find(&key, (void **)&f) == _SHASH_FOUND) {
		if (it_get_from_file(f, data, len) > 0)
			return 0;
	}

	return -1;
}

int ImageTmp::it_get_from_file(const struct image_tmp_value *v, char *data, int *len)
{
	char file[128];

	it_get_fname(file, v->file_idx);

	*len = v->data_len;

	int fd;

	if ((fd = open(file, O_RDONLY)) == -1)
		return -1;

	lseek(fd, v->data_pos, SEEK_SET);
	read(fd, data, v->data_len);

	close(fd);

	return v->data_len;
}

int ImageTmp::it_add_hash(const char *fname, int file_idx, int data_pos, int data_len)
{
	struct image_tmp_key key;
	struct image_tmp_value value;

	memset(&key, 0, sizeof(key));
	memset(&value, 0, sizeof(value));

	strncpy(key.name, fname, sizeof(key.name) - 1);
	value.file_idx = file_idx;
	value.data_pos = data_pos;
	value.data_len = data_len;

	m_hash->sh_replace(&key, &value);

	return 0;
}

int ImageTmp::it_redo_hash()
{
	int len;
	char fname[512];
	char *data = (char *)malloc(5*1024*1024);

	it_reset();

	while (it_read_next(fname, data, &len)) {
		it_add_hash(fname, m_file_read_idx, lseek(m_read_fd, 0, SEEK_CUR) - len, len);
	}

	return 0;
}

int ImageTmp::it_reset()
{
	char file[128];

	m_file_read_idx = 0;

	it_get_fname(file, m_file_read_idx);
	
	if ((m_read_fd = open(file, O_RDONLY | O_BINARY)) == -1)
		return -1;
	
	return 0;
}

int ImageTmp::it_read_next(char *fname, char *data, int *len)
{
	char file[128];
	struct image_tmp_head head;

	if (read(m_read_fd, &head, sizeof(head)) != sizeof(head)) {
		close(m_read_fd);
		m_file_read_idx++;
		it_get_fname(file, m_file_read_idx);
		if (!dashf(file))
			return 0;
		m_read_fd = open(file, O_RDONLY | O_BINARY);

		return it_read_next(fname, data, len);
	}
	if (head.len <= 0)
		return 0;

	strcpy(fname, head.name);
	read(m_read_fd, data, head.len);
	*len = head.len;

	return 1;
}

int ImageTmp::it_add(const char *name, const char *data, int len)
{
	struct image_tmp_head head;

	if (m_curr_size > MAX_IMAGE_TMP_SIZE || m_curr_size == 0) {
		if (it_new_tmp_file() == -1)
			return -1;
	}

	memset(&head, 0, sizeof(head));
	strncpy(head.name, name, sizeof(head.name) - 1);
	head.len = len;

	if (write(m_curr_fd, &head, sizeof(head)) != sizeof(head))
		return -1;

	if (write(m_curr_fd, data, len) == -1) {
		off_t len = file_size(m_curr_fd) - sizeof(head);
		ftruncate(m_curr_fd, len);
		return -1;
	}

	m_curr_size += sizeof(head) + len;

	it_add_hash(name, m_file_idx, m_curr_size - len, len);

	return 0;
}

int ImageTmp::it_new_tmp_file()
{
	char file[128];

	close(m_curr_fd);

	if (m_curr_size == 0 && it_get_last_fname() == 0)
		return 0;

	m_file_idx++;
	it_get_fname(file, m_file_idx);

	if ((m_curr_fd = open(file, O_RDWR | O_BINARY | O_APPEND | O_CREAT, 0644)) == -1)
		return -1;

	m_curr_size = 0;

	return 0;
}

int ImageTmp::it_get_last_fname()
{
	char file[1024];

	it_get_fname(file, 0);

	if (dashf(file)) {
		int i;
		for (i = 0; i < 100; i++) {
			it_get_fname(file, i);
			if (!dashf(file)) {
				m_file_idx = i - 1;
				it_get_fname(file, i - 1);
				break;
			}
		}
	}

	if ((m_curr_fd = open(file, O_RDWR | O_BINARY | O_APPEND | O_CREAT, 0644)) == -1)
		return -1;

	m_curr_size = (int)file_size(m_curr_fd);

	return 0;
}

char *ImageTmp::it_get_fname(char *file, int idx)
{
	sprintf(file, "%s/%s.%d", m_dir, IMAGE_TMP_PREFIX, idx);

	return file;
}


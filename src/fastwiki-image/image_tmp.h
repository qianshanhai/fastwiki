/*
 * Copyright (C) 2014 Qian Shanhai (qianshanhai@gmail.com)
 */
#ifndef __IMAGE_TMP_H
#define __IMAGE_TMP_H

#include "s_hash.h"

struct image_tmp_head {
	char name[80];
	int len;
	int flag;
};

#define IMAGE_TMP_PREFIX "image.tmp"
#define MAX_IMAGE_TMP_SIZE (1024*1024*1024)

struct image_tmp_key {
	char name[256];
};

struct image_tmp_value {
	int file_idx;
	int data_pos;
	int data_len;
};

class ImageTmp {
	private:
		char m_dir[1024];

		int m_curr_fd;
		int m_file_idx;
		int m_curr_size;

		int m_file_read_idx;
		int m_read_fd;

		SHash *m_hash;

	public:
		ImageTmp();
		~ImageTmp();

		int it_init(const char *dir);
		int it_add(const char *name, const char *data, int len);

		int it_reset();
		int it_read_next(char *fname, char *data, int *len);

		int it_new_tmp_file();
		int it_get_last_fname();
		char *it_get_fname(char *file, int idx);

		int it_add_hash(const char *fname, int file_idx, int data_pos, int data_len);
		int it_redo_hash();

		int it_find(const char *fname, char *data, int *len);
		int it_get_from_file(const struct image_tmp_value *v, char *data, int *len);
};

#endif

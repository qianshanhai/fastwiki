/*
 * Copyright (C) 2014 Qian Shanhai (qianshanhai@gmail.com)
 */
#ifndef __WIKI_IMAGE_H
#define __WIKI_IMAGE_H

#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>

#include "s_hash.h"

/*
 * [ head ][ data ][ hash ]
 * hash maybe equal 0
 *
 */

struct wiki_image_head {
	int flag; /* = 0xfa87 */
	int total;
	int file_idx;
	int data_size;
	int hash_size;
	char reserve[64];
};

struct wiki_image_key {
	char fname[160];
};

struct wiki_image_value {
	int file_idx;
	int data_pos;
	int data_len;
	char data_type[16];
	char reserve[16];
};

#include "wiki_data.h"

#define _MAX_PTHREAD_TOTAL 32
#define _MAX_IMAGE_FILE_TOTAL 128

class WikiImage {
	private:
		int m_fd[_MAX_IMAGE_FILE_TOTAL];
		SHash *m_hash;
		SHash *m_fd_hash;
		pthread_mutex_t m_mutex;
		int m_init_flag;

		const char *m_lang;
		int m_curr_fd;
		int m_curr_file_idx;
		int m_curr_size;
		struct wiki_image_head m_head;

		char m_file_name[1024];

		struct wiki_image_value m_value[_MAX_PTHREAD_TOTAL];
		int m_file_pos[_MAX_PTHREAD_TOTAL];

		int m_image_total;

	public:
		WikiImage();
		~WikiImage();

#ifndef FW_NJI
		/* rw */
		int we_new_file();
		int we_add_done();
		int we_write_hash();
		char *we_get_file_name(char *file, int file_idx);

		int we_init(const char *lang, const char *date);
		int we_add_one_image(const char *fname, const char *data, int len);
		int we_output(char *buf);
#endif

		/* ro */
		int we_init(const fw_files_t file, int total);
		int we_reset(int pthread_idx, const char *fname, int *size);
		int we_read_next(int pthread_idx, char *data, int *len);
		int we_pread(int fd, void *data, int len, unsigned int pos);

		/* for test */
		int we_fd_reset();
		int we_fd_read_next(void *key, void *value);
};

#endif

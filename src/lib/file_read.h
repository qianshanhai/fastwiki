/*
 * Copyright (C) 2014 Qian Shanhai (qianshanhai@gmail.com)
 */
#ifndef __WIKI_FILE_READ_H
#define __WIKI_FILE_READ_H

#include <pthread.h>

#include "file_io.h"

#define ONE_BLOCK_SIZE (5*1024*1024)
#define ONE_BLOCK_SIZE_PAD (256*1024)

class FileRead {
	private:
		int m_curr_count;
		int m_debug_count;
		int m_last;
		int m_size;
		char *m_buf;
		pthread_mutex_t m_mutex;
		FileIO *m_file_io;

		int m_end_total;

		int fr_oneblock(char *ret, int *ret_size);

	public:
		FileRead();
		~FileRead();

		int fr_init(const char *file);
		int fr_get_one_block(char *ret, int *ret_size);
		int fr_check_done();
};

#endif

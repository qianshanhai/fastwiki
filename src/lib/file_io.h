/*
 * Copyright (C) 2014 Qian Shanhai (qianshanhai@gmail.com)
 */
#ifndef __FW_FILE_IO_H
#define __FW_FILE_IO_H

#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <zlib.h>
#include <bzlib.h>
#include <stdlib.h>

struct file_io {
	char *data;
	int curr_pos;
	int len;
	int max_size;
};

class FileIO;

class FileIO {
	private:
		int m_fd;
		FILE *m_fp;
		BZFILE *m_bzip;
		gzFile m_gz;

		int (FileIO::*m_read)(void *buf, int size);

		struct file_io m_buf;
		int m_gets_done;
		int m_malloc_flag;

		char *m_lz4_in;
		char *m_lz4_buf;
		int m_lz4_max_buf_size;
		int m_lz4_pos;
		int m_lz4_size;
		int (*m_lz4_func)(const char*, char*, int, int);

	public:
		FileIO();
		~FileIO();

		int fi_init(const char *file, char *buf = NULL, int max_size = 0);
		int fi_init_lz4file(const char *file);
		int fi_read(void *buf, int size);
		int fi_gets(char *buf, int max_size);

		int fi_text_read(void *buf, int len);
		int fi_bzip2_read(void *buf, int size);
		int fi_gzip_read(void *buf, int size);
		int fi_lz4_read(void *buf, int size);
};

#endif

/*
 * Copyright (C) 2014 Qian Shanhai (qianshanhai@gmail.com)
 */
#include <stdlib.h>
#include <stdio.h>
#include <sys/stat.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>

#include "my_strstr.h"
#include "file_read.h"

FileRead::FileRead()
{
	m_end_total = 0;
	m_buf = NULL;

	m_file_io = NULL;
}

FileRead::~FileRead()
{
	if (m_buf)
		free(m_buf);

	if (m_file_io)
		delete m_file_io;
}

int FileRead::fr_init(const char *file)
{
	m_file_io = new FileIO;

	if (m_file_io->fi_init(file) == -1)
		return -1;

	m_buf = (char *)malloc(ONE_BLOCK_SIZE + ONE_BLOCK_SIZE_PAD);

	char *debug = getenv("FW_DEBUG");
	if (debug) {
		m_debug_count = atoi(debug);
	} else
		m_debug_count = 0;

	m_curr_count = 0;
	m_size = m_last = 0;
	pthread_mutex_init(&m_mutex, NULL);

	return 0;
}

int FileRead::fr_get_one_block(char *ret, int *ret_size)
{
	int n, can;

	*ret_size = 0;

	pthread_mutex_lock(&m_mutex);

	if (m_debug_count > 0 && m_curr_count > m_debug_count) {
		m_end_total++;
		pthread_mutex_unlock(&m_mutex);
		return 0;
	}
	m_curr_count++;

	can = ONE_BLOCK_SIZE - m_last;

	if ((n = m_file_io->fi_read(m_buf + m_last, can)) > 0) {
		m_size = m_last + n;
		m_last = fr_oneblock(ret, ret_size);
		pthread_mutex_unlock(&m_mutex);
		return 1;
	}
	m_end_total++;
	
	pthread_mutex_unlock(&m_mutex);

	return 0;
}

int FileRead::fr_oneblock(char *ret, int *ret_size)
{
	int i;
	char *remain = NULL;
	int remain_size = 0;
	int real_size = m_size;

	*ret_size = 0;

	m_buf[ONE_BLOCK_SIZE] = 0;

	for (i = m_size - 1; i >= 0; i--) {
		if (is_page_end(m_buf + i)) {
			remain = m_buf + i + 8;
			*remain++ = 0;
			remain_size = m_size - (remain - m_buf);
			real_size = m_size - remain_size;
			break;
		}
	}

	memcpy(ret, m_buf, real_size);
	ret[real_size] = 0;
	*ret_size = real_size;

	if (remain_size > 0) {
#ifdef DEBUG
		printf("count=%d remain_size=%d, size=%d\n", m_curr_count, remain_size, m_size);
		fflush(stdout);
#endif
		memmove(m_buf, remain, remain_size);
		return remain_size;
	}

	return 0;
}

int FileRead::fr_check_done()
{
	return m_end_total;
}

/*
 * Copyright (C) 2014 Qian Shanhai (qianshanhai@gmail.com)
 */
#define _FILE_OFFSET_BITS 64

#include "file_io.h"
#include "gzip_compress.h"

FileIO::FileIO()
{
	m_fd = -1;
	m_fp = NULL;
	m_bzip = NULL;
	m_gz = NULL;

	memset(&m_buf, 0, sizeof(m_buf));
	m_gets_done = 0;

	m_lz4_pos = 0;
	m_lz4_size = 0;
	m_lz4_in = NULL;
	m_lz4_buf = NULL;
}

FileIO::~FileIO()
{
	if (m_bzip) {
		int err;
		BZ2_bzReadClose(&err, m_bzip);
	}

	if (m_fp)
		fclose(m_fp);

	if (m_gz) {
		gzclose(m_gz);
	}

	if (m_malloc_flag && m_buf.data != NULL) {
		free(m_buf.data);
	}

	if (m_lz4_in)
		free(m_lz4_in);

	if (m_lz4_buf)
		free(m_lz4_buf);
}

int FileIO::fi_init(const char *file, char *buf, int max_size)
{
	int len = strlen(file);

	if (len < 5)
		goto txt;

	if (strncmp(file + len - 4, ".bz2", 4) == 0) {
		int ret;
		if ((m_fp = fopen(file, "rb")) == NULL)
			return -1;
		m_bzip = BZ2_bzReadOpen(&ret, m_fp, 0, 0, NULL, 0);
		m_read = &FileIO::fi_bzip2_read;
	} else if (strncmp(file + len - 3, ".gz", 3) == 0) {
		if ((m_gz = gzopen(file, "rb")) == NULL)
			return -1;
		m_read = &FileIO::fi_gzip_read;
	} else if (strncmp(file + len - 4, ".lz4", 4) == 0) {
		if (fi_init_lz4file(file) == -1)
			return -1;
		m_read = &FileIO::fi_lz4_read;
	} else {
txt:
		if ((m_fd = open(file, O_RDONLY)) == -1)
			return -1;
		m_read = &FileIO::fi_text_read;
	}

	if (buf) {
		m_buf.data = buf;
		m_buf.max_size = max_size;
		m_malloc_flag = 0;
	} else {
		m_buf.max_size = 2*1024*1024;
		m_buf.data = (char *)malloc(m_buf.max_size + 1024);
		m_malloc_flag = 1;
	}

	return 0;
}

int FileIO::fi_gets(char *buf, int max_size)
{
	int read_size = 0;

repeat:
	for (; m_buf.curr_pos < m_buf.len; m_buf.curr_pos++) {
		buf[read_size++] = m_buf.data[m_buf.curr_pos];
		if (m_buf.data[m_buf.curr_pos] == '\n' || read_size >= max_size - 1) {
			buf[read_size] = 0;
			m_buf.curr_pos++;
			return read_size;
		}
	}

	m_buf.curr_pos = 0;

	if (m_gets_done == 0) {
		if ((m_buf.len = fi_read(m_buf.data, m_buf.max_size)) > 0) 
			goto repeat;
		else
			m_gets_done = 1;
	}

	return read_size;
}

int FileIO::fi_read(void *buf, int size)
{
	return  (this->*FileIO::m_read)(buf, size);
}

int FileIO::fi_text_read(void *buf, int len)
{
	return read(m_fd, buf, len);
}

int FileIO::fi_bzip2_read(void *buf, int size)
{
	int err, len;

	len = BZ2_bzRead(&err, m_bzip, buf, size);

	if (err == BZ_OK || BZ_STREAM_END)
		return len;

	return -1;
}

int FileIO::fi_gzip_read(void *buf, int size)
{
	return gzread(m_gz, buf, size);
}

#include "file_io_lz4.h"

int FileIO::fi_init_lz4file(const char *file)
{
	unsigned int magic_number;
	int n;

	if ((m_fp = fopen(file, "rb")) == NULL)
		return -1;

	n = fread(&magic_number, 1, MAGICNUMBER_SIZE, m_fp);
	if (magic_number != LZ4S_MAGICNUMBER)
		return -1;

	unsigned char descriptor[LZ4S_MAXHEADERSIZE];

	if ((n = fread(descriptor, 1, 3, m_fp)) != 3)
		return -1;

	int blockSizeId   = (descriptor[1] >> 4) & _3BITS;
	m_lz4_max_buf_size = LZ4S_GetBlockSize_FromBlockId(blockSizeId) + 80 KB;

	if (m_lz4_max_buf_size < (int)MIN_STREAM_BUFSIZE)
		m_lz4_max_buf_size = MIN_STREAM_BUFSIZE;

	m_lz4_func = LZ4_decompress_safe;

	if (!((descriptor[0] >> 5) & _1BIT)) {
		m_lz4_func = LZ4_decompress_safe_withPrefix64k;
	}

	m_lz4_in = (char *)malloc(m_lz4_max_buf_size);
	m_lz4_buf = (char*)malloc(m_lz4_max_buf_size);

	return 0;
}

int FileIO::fi_lz4_read(void *buf, int size)
{
	int n, read_size = 0, remain = size;
	unsigned int block_size;

	while (1) {
		if (m_lz4_size - m_lz4_pos > 0) {
			if (m_lz4_size - m_lz4_pos >= remain) {
				if (remain > 0) {
					memcpy((char *)buf + read_size, m_lz4_buf + m_lz4_pos, remain); 
					m_lz4_pos += remain;
				}
				return size;
			} else {
				memcpy((char *)buf + read_size, m_lz4_buf + m_lz4_pos, m_lz4_size - m_lz4_pos);
				read_size += m_lz4_size - m_lz4_pos;
				remain -= read_size;
			}
		}

		m_lz4_pos = 0;
		m_lz4_size = 0;

		if ((n = fread(&block_size, 1, 4, m_fp)) != 4)
			break;

		if (block_size == LZ4S_EOS)
			break;

		block_size = LITTLE_ENDIAN_32(block_size);   // Convert to little endian
		block_size &= 0x7FFFFFFF;

		n = fread(m_lz4_in, 1, block_size, m_fp);
		m_lz4_size = m_lz4_func(m_lz4_in, m_lz4_buf, block_size, m_lz4_max_buf_size);

		if (m_lz4_size < 0)
			break;
	}

	return read_size;
}

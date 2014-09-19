/*
 * Copyright (C) 2014 Qian Shanhai (qianshanhai@gmail.com)
 */

#include <fcntl.h>
#include "q_util.h"
#include "crc32sum.h"

#include "httpd-index.h"

#define _HTTPD_INDEX_TMP_DIR "tmp"

HttpdIndex::HttpdIndex()
{
	m_full_index = NULL;
}

HttpdIndex::~HttpdIndex()
{
}

int HttpdIndex::hi_init(WikiFullIndex *full_index)
{
	if (!dashd(_HTTPD_INDEX_TMP_DIR)) {
		if (q_mkdir(_HTTPD_INDEX_TMP_DIR, 0755) == -1)
			return -1;
	}
	
	m_full_index = full_index;

	return 0;
}

int HttpdIndex::hi_new_fetch(const char *file, const char *key, int *page, int *page_idx, int *all_total)
{
	int fd, read_total, ret_total;

	if ((fd = open(file, O_RDWR | O_CREAT | O_TRUNC | O_BINARY, 0644)) == -1)
		return -1;

	ret_total = m_full_index->wfi_find(key, m_page_idx, _HTTPD_INDEX_MAT_TOTAL, all_total);

	write(fd, all_total, sizeof(int));
	write(fd, &ret_total, sizeof(ret_total));

	if (ret_total > 0)
		write(fd, m_page_idx, ret_total * sizeof(int));

	close(fd);

	if (ret_total == 0)
		return 0;

	read_total = hi_get_page_total(page, ret_total);
	memcpy(page_idx, &m_page_idx[(*page - 1) * _HTTPD_ONE_PAGE_TOTAL], read_total * sizeof(int));

	return read_total;
}

int HttpdIndex::hi_old_fetch(const char *file, const char *key, int *page, int *page_idx, int *all_total)
{
	int n, fd, ret_total, read_total;

	if ((fd = open(file, O_RDONLY | O_BINARY)) == -1)
		return -1;

	*all_total = 0;
	ret_total = 0;

	read(fd, all_total, sizeof(int));
	read(fd, &ret_total, sizeof(ret_total));

	read_total = hi_get_page_total(page, ret_total);

	lseek(fd, (*page - 1) * _HTTPD_ONE_PAGE_TOTAL * sizeof(int) + 8, SEEK_SET);
	n = read(fd, page_idx, read_total * sizeof(int));
	close(fd);
	
	if (n != read_total * (int)sizeof(int))
		return 0;

	return read_total;
}

int HttpdIndex::hi_fetch(const char *key, int *page, int *page_idx, int *all_total)
{
	int n;
	unsigned int crc32;
	char file[128], dir[256], word[256];

	if (m_full_index->wfi_init_is_done() == 0)
		return 0;

	memset(word, 0, sizeof(word));
	strncpy(word, key, sizeof(word) - 1);

	trim(word);
	q_tolower(word);

	/* TODO convert key */
	crc32 = crc32sum(word);

	sprintf(dir, "%s/%u/%u", _HTTPD_INDEX_TMP_DIR, crc32 % 13, crc32 % 101);
	
	if (!dashd(dir)) {
		if (q_mkdir(dir, 0755) == -1)
			return -1;
	}

	sprintf(file, "%s/%u", dir, crc32);

	if (!dashf(file))
		n = hi_new_fetch(file, word, page, page_idx, all_total);
	else
		n = hi_old_fetch(file, word, page, page_idx, all_total);

	return n;
}

int HttpdIndex::hi_get_page_total(int *page, int total)
{

	int page_total = _page_total(total);

	if (*page <= 0)
		*page = 1;

	if (*page > page_total)
		*page = page_total;

	int read_total = total - (*page - 1) * _HTTPD_ONE_PAGE_TOTAL;

	if (read_total > _HTTPD_ONE_PAGE_TOTAL)
		read_total = _HTTPD_ONE_PAGE_TOTAL;

	return read_total;
}

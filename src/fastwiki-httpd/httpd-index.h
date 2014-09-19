/*
 * Copyright (C) 2014 Qian Shanhai (qianshanhai@gmail.com)
 */

#ifndef __FW_HTTPD_INDEX_H
#define __FW_HTTPD_INDEX_H

#include "wiki_full_index.h"

#define _HTTPD_INDEX_MAT_TOTAL (10*10000)
#define _HTTPD_ONE_PAGE_TOTAL 10

#define _page_total(_t) \
	((_t) % _HTTPD_ONE_PAGE_TOTAL ? (_t) / _HTTPD_ONE_PAGE_TOTAL + 1 : (_t) / _HTTPD_ONE_PAGE_TOTAL)

class HttpdIndex {
	private:
		WikiFullIndex *m_full_index;
		int m_page_idx[_HTTPD_INDEX_MAT_TOTAL];

	public:
		HttpdIndex();
		~HttpdIndex();

	public:
		int hi_init(WikiFullIndex *full_index);
		int hi_fetch(const char *key, int *page, int *page_idx, int *all_total);

	private:
		int hi_old_fetch(const char *file, const char *key, int *page, int *page_idx, int *all_total);
		int hi_new_fetch(const char *file, const char *key, int *page, int *page_idx, int *all_total);
		int hi_get_page_total(int *page, int total);
};

#endif

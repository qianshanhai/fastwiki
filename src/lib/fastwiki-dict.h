/*
 * Copyright (C) 2014 Qian Shanhai (qianshanhai@gmail.com)
 */
#ifndef __FASTWIKI_DICT_H
#define __FASTWIKI_DICT_H

#include <stdio.h>

class FastwikiDict {
	public:
		FastwikiDict();
		~FastwikiDict();

		int dict_init(const char *mark, const char *date, const char *compress_flag);

		int dict_add_title(const char *title, int len = 0);
		int dict_add_title_done();

		int dict_add_page(const char *page, int page_size, const char *title, int title_len = 0,
					const char *redirect = NULL, int redirect_len = 0);

		int dict_output();
		int dict_find_title(const char *key, int len);
};

#endif

/*
 * Copyright (C) 2014 Qian Shanhai (qianshanhai@gmail.com)
 */

#ifndef __FASTWIKI_TEXT_H
#define __FASTWIKI_TEXT_H

typedef int (*text_parse_func_t)(const char *content, int len);

extern "C" {
	int text_find_title(const char *title, int len);
	char *text_curr_title(int *len);
	char *text_curr_content(int *len);
	int text_add_one_title(const char *content, int len);
	int text_add_one_content(const char *content, int len);
	int text_add_one_file(const char *file, text_parse_func_t func, int flag);

	int text_add_one_file_title(const char *file, int not_used);
	int text_add_one_file_page(const char *file, int not_used);

	int ft_scan_dir(const char *dir, split_t &sp, int (*func)(const char *file, text_parse_func_t f, int flag),
			text_parse_func_t parse, int flag);
};

#endif

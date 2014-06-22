/*
 * Copyright (C) 2014 Qian Shanhai (qianshanhai@gmail.com)
 */
#ifndef __WIKI_PARSE_H
#define __WIKI_PARSE_H

#include "wiki_math.h"
#include "wiki_memory.h"
#include "wiki_zh.h"
#include "image_fname.h"

/*
 * parse one block
 */

#define _SPLIT_TMP_CH 0x1

#define MAX_CHAPTER_INDEX 128

#define PARSE_ALL_FLAG 0x1
#define PARSE_TITLE_FLAG 0x2

struct wiki_parse {
	char *ref_buf;
	int ref_size;
	int ref_count;
	int ref_flag;
	char *chapter;
	int chapter_size;
	int chapter_index[MAX_CHAPTER_INDEX];
	int chapter_count;
	char *tmp;
	char *zip;
	char *math;
	int parse_flag;
};

typedef int (*add_page_func_t)(int flag, const char *page, int page_z_size, int page_old_size,
		const char *_title, int len, const char *redirect, int n);
typedef int (*find_key_func_t)(const char *key, int len);

class WikiParse {
	private:
		add_page_func_t m_add_page;
		find_key_func_t m_find_key;

		int m_complete_flag;
		int m_zh_flag;

		WikiMath *m_wiki_math;
		WikiMemory *m_mem;
		WikiZh *m_wiki_zh;
		ImageFname *m_image_fname;

		int (*m_compress_func)(char *out, int out_len, const char *in, int in_len);

	public:
		WikiParse();
		~WikiParse();

		int wp_init(add_page_func_t _add_page, find_key_func_t _find_key, int zh_flag, int z_compress_flag);
		int wp_do(char *buf, int size, struct wiki_parse *wp);
		int wp_ref_init(struct wiki_parse *wp);

		int wp_get_title(char *p, char *title, char *redirect);

		int wp_is_lang_key(char *key);

		int wp_do_parse(char *buf, int size, char *out, int *out_size, int max_out_size, struct wiki_parse *wp);
		int wp_do_parse_sys(char *start, int size, const char *title, const char *redirect, struct wiki_parse *wp);

		int wp_is_italic(char *p, int max_size, char *out, int *out_size, int *do_len, struct wiki_parse *wp);
		int wp_is_http(char *p, int max_size, char *out, int *out_size, int *do_len, struct wiki_parse *wp);
		int wp_is_link(char *p, int max_size, char *out, int *out_size, int *do_len, struct wiki_parse *wp);
		int wp_is_comment(char *p, int max_size, char *out, int *out_size, int *do_len, struct wiki_parse *wp);
		int wp_is_ref(char *p, int max_size, char *out, int *out_size, int *do_len, struct wiki_parse *wp);

		inline int wp_table_one_item(char *t, const char *sub, const char *th,
				char *out, int *out_size, struct wiki_parse *wp);
		int wp_do_table(char *p, int size, char *out, int *out_size, struct wiki_parse *wp, const char *newline);
		int wp_is_table(char *p, int max_size, char *out, int *out_size, int *do_len, struct wiki_parse *wp);

		int wp_is_math(char *p, int max_size, char *out, int *out_size, int *do_len, struct wiki_parse *wp);
		int wp_is_chatper(char *p, int max_size, char *out, int *out_size, int *do_len, struct wiki_parse *wp);
		int wp_is_brackets(char *p, int max_size, char *out, int *out_size, int *do_len,struct wiki_parse *wp);
		int wp_is_file(char *p, int max_size, char *out, int *out_size, int *do_len, struct wiki_parse *wp);
		int wp_is_file2(char *p, int max_size, char *out, int *out_size, int *do_len, struct wiki_parse *wp);
		int wp_do_file(int flag, char *p, int max_size, char *out, int *out_size, struct wiki_parse *wp);
		int wp_is_source_code(char *p, int max_size, char *out, int *out_size, int *do_len, struct wiki_parse *wp);
		int wp_is_list(char *p, int max_size, char *out, int *out_size, int *do_len, struct wiki_parse *wp);
		int wp_is_dd(char *p, int max_size, char *out, int *out_size, int *do_len, struct wiki_parse *wp);
};

#endif


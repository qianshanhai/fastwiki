/*
 * Copyright (C) 2014 Qian Shanhai (qianshanhai@gmail.com)
 */
/*
 * index file struct:
 *                 4M           16M           15M          
 * [ wi_head ] [ href_idx ] [ sort_idx ] [ key_string ] [ key store ] [ hash ]
 * wi_head = struct wi_head
 * href_idx = [ int int ... ]
 * sort_idx = [ sort_idx_t ][ sort_idx_t ] ...
 * key_string = [ key1 ][ key2 ][ key3 ] ...
 * key store:
 *    [key1][total][idx1 idx2 idx3 ...] [key2][total][idx1 idx2 idx3 ...] ...
 * hash: a SHash
 *   key = key store's key
 *   value = key store pos
 */

#ifndef __WIKI_INDEX_H
#define __WIKI_INDEX_H

#include "q_util.h"
#include "s_hash.h"

enum {
	WK_INDEX_FIND = 0,
	WK_KEY_FIND,
	WK_MATCH_FIND,
	WK_LIKE_FIND,
};

#define DATA_LEN_1K 1024

typedef struct sort_idx {
	unsigned int key_pos; /* start with key_string, after sort_idx */
	unsigned char key_len;
	unsigned char data_file_idx; /* data file index, use on more than one data file */
	unsigned short data_len; /* 1K */
	unsigned int data_pos; /* */
	unsigned char flag[8];
} sort_idx_t;

typedef struct wi_head {
	unsigned char flag;
	char name[23];
	int version;			/* >= version */
	char random[32];
	int href_idx_total;
	int href_idx_size;
	int sort_idx_total; /* */
	int sort_idx_size;
	int key_string_size; /* */
	int key_store_total; /* */
	int key_store_size;
	int hash_size;
	unsigned int file_size;
	char reserve[64];
	unsigned int crc32; /* = crc32(head, sizeof(wi_head) - 4) */
} wi_head_t;

#define MAX_KEY_LEN 64
#define KEY_SPLIT_LEN 32

struct key_store {
	char key[24];
};

struct key_store_head {
	char key[24];
	int total;
	int value[1];
};

struct hash_v {
	unsigned int pos;
	int start_idx; /* sort_idx[] */
	int end_idx;   /* sort_idx[] */
};

#define MAX_TMP_IDX_TOTAL 150000

#define MAX_FIND_RECORD 128

class WikiIndex;

class WikiIndex {
	private:
		/* output */
		FILE *m_fp;
		SHash *m_title_hash;
		int m_idx_total;
		int m_key_len;
		char *m_tmp;
		int m_key_total;

		sort_idx_t *m_sort_idx;
		char *m_key_string;
		char *m_key_store; 


		/* find */
		wi_head_t *m_wi_head;

		int m_fd;
		sort_idx_t m_curr_sort_idx;
		char m_curr_key[1024];

		int (WikiIndex::*m_search_func)(int start, int i, 
				const char *key, int key_len, sort_idx_t *ret_idx, int *total);
		int m_search_flag;

		SHash *m_hash;

		int *m_tmp_idx; /* max = MAX_TMP_IDX_TOTAL */

		int (*m_check_data_func)(int data_file_idx);

	public:
		WikiIndex();
		~WikiIndex();

		int wi_output_init();
		int wi_title_find(const char *title, int len);
		int wi_one_title(const char *title, int len);
		int wi_add_title_done();
		int wi_add_key_to_hash(const char *title, int len, int idx, unsigned char flag[8]);
		int wi_add_one_page(const char *title, int len, const char *redirect, int r_len,
				unsigned char data_file_idx, unsigned short data_len, unsigned int data_pos);
		int wi_output(const char *outfile);
		int wi_set_head(wi_head_t *h, int idx_total, int key_len, int key_total,
				int store_len, int hash_len);
		int wi_write_index_file(const char *outfile, const wi_head_t *h, const unsigned int *href_idx,
				sort_idx_t *sort_idx, const char *key_string, const char *store, const char *hash);

	public:

		int wi_init(const char *file);
		int wi_set_data_func(int (*func)(int data_file_idx));
		int wi_find(const char *key, int flag, int index, sort_idx_t *idx, int *idx_len);
		int wi_stat();

	public:
		int wi_create_vhash(sort_idx_t *sort_idx, int total, char *key_string,
				char **store, int *store_len, int *key_total, char **h, int *h_len);
		int wi_match(const char *key, sort_idx_t *idx, int *idx_len);
		int wi_binary_find(const int *to, int total, int v);
		int wi_cmp_two_idx(int *f, int f_total, const int *to, int to_total);
		int wi_search(const char *key, int flag, sort_idx_t *ret_idx, int *total);
		int wi_search2(const char *key, int key_len, int flag, int idx, sort_idx_t *ret_idx, int *total);
		int wi_split_key(const char *key, int k_len, char ret[128][MAX_KEY_LEN], int *len);
		int wi_get_key(const sort_idx_t *idx, char *key);
		int wi_get_key_href(int href_idx, char *key);
		int wi_check_search(const char *key, int *low, int *high);
		int wi_user_search(int start, int i, const char *key, int key_len, sort_idx_t *ret_idx, int *total);
		int wi_sys_search(int start, int i, const char *key, int key_len, sort_idx_t *ret_idx, int *total);

		inline char *wi_get_key_string(const sort_idx_t *idx);
		inline sort_idx_t *wi_get_sort_idx(int idx);
		inline int wi_get_href_idx(int idx);
		unsigned int wi_get_href_idx_sys(int idx);

		int wi_set_key_flag(char *key, unsigned char *flag);
		int wi_unset_key_flag(const char *old, char *key, unsigned const char *flag);
		int wi_unset_key_flag(char *key, unsigned const char *flag);
		sort_idx_t *wi_random_key(sort_idx_t *idx);

		int wi_href_total();
		int wi_fetch(int index, sort_idx_t *idx);
};

#ifdef __cplusplus
extern "C" {
#endif

int my_strcmp(const char *p1, const char *p2, size_t n);
int my_nstrcmp(const char *p1, const char *p2, size_t n);

#ifdef __cplusplus
};
#endif

#endif

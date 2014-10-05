/*
 * Copyright (C) 2014 Qian Shanhai (qianshanhai@gmail.com)
 */

#ifndef __WIKI_FULL_INDEX_H
#define __WIKI_FULL_INDEX_H

#include <pthread.h>

#include "wiki_common.h"
#include "s_hash.h"

/*
 * full index have many index, name as fastwiki.fidx.zh.0
 * first fidx file format:
 *
 * [ head ][ page_idx ][ hash ][ value ][ string ][ reverse idx ][ data ]
 *
 * the other fidx file format:
 * [ head ][ data ]
 *
 */

#define _WFI_CHECK_WORD_LEN 3 /* check a mutil-byte word len */
#define _WFI_IDX_TITLE_LEN 24

struct fidx_head {
	char name[32];  /* ".fastwiki.full.text.search" */
	int version;
	int z_flag;  /* compress flag */
	char random[32]; /* same as fastwiki.dat and fastwiki.idx */
	char lang[32];   /* same as fastwiki.dat and fastwiki.idx */
	int file_idx;
	int bitmap_size;
	int page_index_flag; /* =1 page index equal inline index */
	int page_index_total;
	int value_total;
	unsigned int hash_pos;
	unsigned int hash_size;
	unsigned int value_pos; /* value_size = value_total * sizeof(idx_value_t) */
	unsigned int string_pos;
	unsigned int string_len;
	unsigned int reverse_index_pos;
	char reserve[256];
	unsigned int head_crc32; /* Not used now */
	unsigned int crc32;      /* Not used now */
};

struct fidx_key {
	char word[_WFI_IDX_TITLE_LEN];
};

struct fidx_value {
	unsigned int pos;
	unsigned int len;
	unsigned char file_idx;
	unsigned char text_flag;
	char r[2];
};

typedef struct {
	unsigned int pos;
	unsigned int len;
	unsigned char file_idx;
	unsigned char text_flag;
	char r[1];
	unsigned char word_len;
	unsigned int word_pos;
} idx_value_t;

struct wfi_tmp_key {
	char word[_WFI_IDX_TITLE_LEN];
};

struct wfi_tmp_value {
	unsigned int total;
	unsigned int start_rec_idx; /* point to index of m_tmp_rec */
	unsigned int last_rec_idx;
};

struct wfi_tmp_max {
	struct wfi_tmp_key key;
	struct wfi_tmp_value *v;
};

/*
  tmp_mem structure
  [ wfi_tmp_head ][ deleted ][ rec 1 ] [ rec 2]
  rec = wfi_tmp_rec
*/

struct wfi_tmp_head {
	unsigned int max_total;
	unsigned int used;
	unsigned int deleted_total;
	unsigned int deleted[1];
};

#define _MAX_TMP_PAGE_IDX_TOTAL 32

struct wfi_tmp_rec {
	unsigned int page_idx[_MAX_TMP_PAGE_IDX_TOTAL];
	int total;
	unsigned int next;
};

struct wfi_curr_pos {
	unsigned int curr_pos;
	int fd;
	int curr_fd_idx;
};

struct tmp_key_val {
	struct wfi_tmp_key key;
	struct wfi_tmp_value val;
};

struct word_key {
	char word[_WFI_IDX_TITLE_LEN];
};

struct word_value {
	int pos; /* point to idx_value_t: m_value[pos] */
};

class WikiFullIndex {
	public:
		WikiFullIndex();
		~WikiFullIndex();

	/* use for ro */
	private:
		int m_fd[128];
		SHash *m_hash;
		struct fidx_head m_head;

		int m_ro_init_flag; /* =1 for initial succuse, =0 for fail */

		char *m_comp_buf;
		int m_comp_buf_len; /* = m_head.bitmap_size + 128K */

		compress_func_t m_decompress_func;

	private:
		int wfi_convert_index(int idx);
		int wfi_reverse_idx(int idx);
		int wfi_get_value(int idx, idx_value_t *v);

	public:
		int wfi_init(const fw_files_t file, int total);
		int wfi_init_is_done();

		/*
		 * string use space to split, such as "a b c"
		 */
		int wfi_find(const char *string, int *page_idx, int max_total, int *count_total = NULL);

		int wfi_unload_all_word(const char *file);
		int wfi_stat();
		int wfi_count(const char *file);

	/* use for rw */
	private:
		int m_index_total; /* total of page */
		int m_bitmap_size;          /* size = m_index_total / 8 + 1 */
		unsigned char *m_bitmap;    /* also used for ro */
		unsigned char *m_tmp_buf;   /* also used for ro */

		char m_tmp_dir[128];
		char m_lang[24];

		void *m_tmp_mem;
		struct wfi_tmp_head *m_tmp_head;
		struct wfi_tmp_rec *m_tmp_rec;

		int *m_page_index;
		int m_page_index_pos;

		SHash *m_word_hash;

		SHash *m_tmp_hash;
		SHash *m_flush_hash;

		pthread_mutex_t m_mutex;
		SHash *m_key_hash[128];
		int m_pthread_total;

		int m_flush_fd[128];
		unsigned int m_flush_size[128];
		int m_curr_flush_fd_idx;

		compress_func_t m_compress_func;

		int m_z_flag;
		struct wfi_curr_pos m_wfi_curr_pos;

		int m_word_total;
		int m_word_str_len;
		int *m_reverse_index;
		idx_value_t *m_value;
		int m_tmp_fd;
		char *m_string;

		int m_min_compress_total;
		int *m_min_page_idx;

	public:
		int wfi_create_init(const char *z_flag, int index_total, const char *lang,
				const char *tmp_dir, size_t mem_size, SHash *word_hash,
				int pthread_total = 1);
		int wfi_add_page(int page_idx, const char *page, int page_len, int pthread_idx = 0);
		int wfi_add_page_done();
		int wfi_flush_data_one_pthread();

	private:
		int wfi_add_one_word(const char *word, int page_idx);

		int wfi_init_tmp_mem(size_t mem_size);
		int wfi_new_tmp_rec(unsigned int last_idx, unsigned int page_idx, unsigned int *new_last_idx);
		int wfi_delete_tmp_rec(unsigned int start_idx);
		int wfi_tmp_get_max_k(struct wfi_tmp_max *m, int k);

		int wfi_update_one_bitmap(unsigned int idx, unsigned char *bitmap, int *min_page);
		int wfi_read_file_to_one_bitmap(unsigned char *buf, int size, struct fidx_value *f,
				int *min_page, void *tmp1, void *tmp2);
		int wfi_flush_one_data(struct wfi_tmp_key *k, struct wfi_tmp_value *v);
		int wfi_write_one_bitmap_to_file(const void *buf, int len, struct fidx_value *v, int flag);

		int wfi_flush_some_data();
		int wfi_flush_all_data();

		const char *wfi_get_fidx_fname(char *file, const char *lang, int idx);
		int wfi_new_index_file(int curr_fd_idx);

		int wfi_flush_full_index();
		int wfi_rewrite_file_head(const struct fidx_head *h);
		int wfi_check_inline_index();

		int wfi_set_head(struct fidx_head *h);
		int wfi_recount_value_file_idx();
		int wfi_create_idx_value(int word_len);
		int wfi_sort_tmp_record();
		int wfi_write_tmp_hash(int *word_len);
		int wfi_count_min_compress_size();
};

#endif

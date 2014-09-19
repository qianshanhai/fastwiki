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
 * the first fidx file have SHash on the file start:
   [ SHash ][ ... ]
 
   SHash key  :
   char word[64];
   SHash value:  
 
 findex 0
 findex 1
 */

#define _WFI_CHECK_WORD_LEN 3 /* check a mutil-byte word len */

#define _WFI_IDX_TITLE_LEN 64

struct fidx_head {
	int z_flag;  /* compress flag */
	int hash_pos;
	int hash_size;
	int file_idx;
	int bitmap_size;
	int index_flag; /* =1 page index equal inline index */
	char random[32];
	char lang[32];
	char reserve[256];
	unsigned int head_crc32; /* Not used now */
	unsigned int crc32;      /* Not used now */
};

struct fidx_key {
	char word[_WFI_IDX_TITLE_LEN];
};

struct fidx_value {
	unsigned long long pos;
	unsigned int len;
	unsigned char file_idx;
	char r3[2];
};

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

	public:
		int wfi_init(const fw_files_t file, int total);
		int wfi_init_is_done();

		/*
		 * string use space to split, such as "a b c"
		 */
		int wfi_find(const char *string, int *page_idx, int max_total, int *count_total = NULL);

		int wfi_unload_all_word(const char *file);
		int wfi_stat();

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

	public:
		int wfi_create_init(const char *z_flag, int index_total, const char *lang,
				const char *tmp_dir, size_t mem_size, SHash *word_hash,
				int pthread_total = 1);
		int wfi_add_page(int page_idx, const char *page, int page_len, int pthread_idx = 0);
		int wfi_add_page_done();

	private:
		int wfi_add_one_word(const char *word, int page_idx);

		int wfi_init_tmp_mem(size_t mem_size);
		int wfi_new_tmp_rec(unsigned int last_idx, unsigned int page_idx, unsigned int *new_last_idx);
		int wfi_delete_tmp_rec(unsigned int start_idx);
		int wfi_tmp_get_max_k(struct wfi_tmp_max *m, int k);

		int wfi_update_one_bitmap(unsigned int idx, unsigned char *bitmap);
		int wfi_read_file_to_one_bitmap(unsigned char *buf, int size, struct fidx_value *f);
		int wfi_flush_one_data(struct wfi_tmp_key *k, struct wfi_tmp_value *v);
		int wfi_write_one_bitmap_to_file(const void *buf, int len, struct fidx_value *v);

		int wfi_flush_some_data();
		int wfi_flush_all_data();

		const char *wfi_get_fidx_fname(char *file, const char *lang, int idx);
		int wfi_new_index_file(int curr_fd_idx);

		int wfi_flush_full_index();
		int wfi_rewrite_file_head(const struct fidx_head *h);
		int wfi_check_inline_index();
};

#endif

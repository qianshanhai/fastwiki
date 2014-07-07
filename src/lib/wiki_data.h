/*
 * Copyright (C) 2014 Qian Shanhai (qianshanhai@gmail.com)
 */
#ifndef __WIKI_DATA_H
#define __WIKI_DATA_H

#include <stdio.h>
#include "wiki_common.h"

/*
 * data file struct
 * [ head ] [ record_head ] [ ra1 ][ ra2 ][ ran ] [ record_head ] [ rb1 ][ rb2 ][ rbn]
 *
 * head = struct data_head
 * record_head = struct data_record_head
 * r1 = one record, maybe text or compress with bzip2, gzip
 *
 * record_head 
 *  <...>[ ]
 *  <img src="http://127.0.0.1:xxxx/a.b.c.png" alt="..." /> 
 *  a = length(math);
 *  b = crc32(math);
 *  c = r_crc32(math);
 * 
 */

typedef struct data_head {
	unsigned char flag;
	char name[23];
	int version2;
	char random[32];
	unsigned int magic;
	unsigned int file_size;   /* file_size */
	unsigned int page_count;  /* */
	int max_old_len;
	int z_flag;
	char date[16];
	char lang[32];
	char version[16];
	char reverse[212];
	int complete_flag;  /* =1 complete */
	int end_file_flag;  /* >0 last data file */
	unsigned int file_idx;    /* data_file_idx, start with = 0 */
	unsigned int crc32; /* = crc32(head, sizeof(data_head) - 4) */
} data_head_t;

/*
 * 2.0.x
 */
typedef struct {
	int out_len;
	int old_len; /* old len */
} old_data_record_head_t;

/*
 * 2.1.x
 */
typedef struct data_record_head {
	int out_len;
	int old_len; /* old len */
	int flag; 	 /* no defined */
} data_record_head_t;

#define OUT_SIZE (3*1024*1024)

typedef char fw_files_t[MAX_FD_TOTAL][128];

class WikiData {
	private:
		FILE *m_data_fp;
		data_head_t m_head; 
		char *m_tmp;
		int m_out_size;
		unsigned int m_page_count;
		unsigned int m_page_size;

		int m_data_file_total;
		int m_z_flag;
		char m_date[64];
		char m_lang[32];
		char m_fname[64];

		char *m_buf;
		int m_fd_idx[MAX_FD_TOTAL];
		int m_fd_total;

		char **m_fname_list;
		int m_fname_total;

	public:
		WikiData();
		~WikiData();

		int wd_init(const fw_files_t file, int total);
		int wd_check_fd(int data_file_idx);
		//int wd_read(int file_idx, unsigned int pos, int len, char **ret);
		int wd_sys_read(int file_idx, unsigned int pos, int len, char *ret, int ret_max);

		int wd_output_current_file();
		int wd_new_file();
		void wd_init_data_head(data_head_t *p);
		int wd_init(const char *outfile, int z_flag, const char *date, const char *creator);

		int wd_add_one_page(int flag, const char *page, int page_z_size, int page_old_size,
					int *ret_file_idx, unsigned int *ret_pos, int *ret_size);
		int wd_output();
		int wd_file_list(char *fname_list);
		int wd_get_head(data_head_t *h);
};

#endif
